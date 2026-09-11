// asset_system_example.cpp
// Compile: g++ -std=c++17 asset_system_example.cpp -pthread -O2 -o asset_system_example
#include <iostream>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <future>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <optional>

// ---------------------------------------------------
// Basic Resource types
// ---------------------------------------------------
struct Resource
{
    std::string id; // unique id (path/name)
    std::atomic<int> refCount{0}; // optional manual ref count for debug
    Resource(const std::string& id_) : id(id_) {}
    virtual ~Resource() {
        std::cout << "[Resource] destroyed: " << id << "\n";
    }
    virtual void debugInfo() const {
        std::cout << "Resource: " << id << "\n";
    }
};

struct TextureResource : Resource
{
    int width = 0;
    int height = 0;
    std::vector<uint8_t> pixels; // dummy
    TextureResource(const std::string& id_, int w, int h)
        : Resource(id_), width(w), height(h) {
        pixels.resize(w * h * 4);
        std::cout << "[TextureResource] created: " << id << " (" << w << "x" << h << ")\n";
    }
    ~TextureResource() override {
        std::cout << "[TextureResource] destroyed: " << id << "\n";
    }
    void debugInfo() const override {
        std::cout << "Texture: " << id << " (" << width << "x" << height << ")\n";
    }
};

// ---------------------------------------------------
// AssetHandle: "hard" reference (shared_ptr wrapper)
// ---------------------------------------------------
template<typename T>
class AssetHandle
{
public:
    AssetHandle() = default;
    AssetHandle(std::shared_ptr<T> ptr) : ptr_(ptr) {
        if (ptr_) ++ptr_->refCount;
    }
    AssetHandle(const AssetHandle& other) : ptr_(other.ptr_) {
        if (ptr_) ++ptr_->refCount;
    }
    AssetHandle(AssetHandle&& other) noexcept : ptr_(std::move(other.ptr_)) {}
    AssetHandle& operator=(const AssetHandle& other) {
        if (this != &other) {
            reset();
            ptr_ = other.ptr_;
            if (ptr_) ++ptr_->refCount;
        }
        return *this;
    }
    AssetHandle& operator=(AssetHandle&& other) noexcept {
        if (this != &other) {
            reset();
            ptr_ = std::move(other.ptr_);
        }
        return *this;
    }
    ~AssetHandle() { reset(); }

    T* get() const { return ptr_.get(); }
    std::shared_ptr<T> shared() const { return ptr_; }
    bool isValid() const { return ptr_ != nullptr; }

    void reset() {
        if (ptr_) {
            --ptr_->refCount;
            ptr_.reset();
        }
    }

    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_.get(); }

private:
    std::shared_ptr<T> ptr_;
};

// ---------------------------------------------------
// ResourceManager: load/unload, registry, async load
// ---------------------------------------------------
class ResourceManager
{
public:
    static ResourceManager& Get() {
        static ResourceManager inst;
        return inst;
    }

    // Sync load (if exists in registry, returns existing; otherwise loads via loader)
    template<typename T, typename LoaderFunc>
    AssetHandle<T> LoadSync(const std::string& id, LoaderFunc loader) {
        std::unique_lock lock(mutex_);
        // check existing
        auto it = registry_.find(id);
        if (it != registry_.end()) {
            if (auto existing = it->second.lock()) {
                // try cast
                if (auto casted = std::dynamic_pointer_cast<T>(existing)) {
                    return AssetHandle<T>(casted);
                } else {
                    // type mismatch
                    std::cerr << "[ResourceManager] Type mismatch when loading " << id << "\n";
                    return {};
                }
            }
        }
        // not loaded -> create
        lock.unlock(); // run loader without lock (loader may be heavy)
        auto created = loader();
        lock.lock();
        registry_[id] = created;
        return AssetHandle<T>(std::dynamic_pointer_cast<T>(created));
    }

    // Async load returns a future holding AssetHandle<T>.
    // loader should be a callable that returns shared_ptr<Resource> (created object)
    template<typename T, typename LoaderFunc>
    std::future<AssetHandle<T>> LoadAsync(const std::string& id, LoaderFunc loader) {
        // We avoid blocking the caller: start async task
        return std::async(std::launch::async, [this, id, loader=std::move(loader)]() -> AssetHandle<T> {
            // First check registry fast-path
            {
                std::lock_guard lock(mutex_);
                auto it = registry_.find(id);
                if (it != registry_.end()) {
                    if (auto existing = it->second.lock()) {
                        if (auto casted = std::dynamic_pointer_cast<T>(existing)) {
                            return AssetHandle<T>(casted);
                        }
                    }
                }
            }
            // Not present -> run loader (simulate IO)
            auto created = loader();
            {
                std::lock_guard lock(mutex_);
                registry_[id] = created;
            }
            return AssetHandle<T>(std::dynamic_pointer_cast<T>(created));
        });
    }

    // Try resolve an entry if already loaded, else null handle.
    template<typename T>
    AssetHandle<T> TryResolve(const std::string& id) {
        std::lock_guard lock(mutex_);
        auto it = registry_.find(id);
        if (it == registry_.end()) return {};
        if (auto existing = it->second.lock()) {
            if (auto casted = std::dynamic_pointer_cast<T>(existing)) {
                return AssetHandle<T>(casted);
            }
        }
        return {};
    }

    // Manual unload hint: remove weak_ptr from registry (actual deletion occurs when shared_ptr refcount==0)
    void Unregister(const std::string& id) {
        std::lock_guard lock(mutex_);
        registry_.erase(id);
    }

    // Debug: print registry (entries that still exist)
    void DebugPrintRegistry() {
        std::lock_guard lock(mutex_);
        std::cout << "=== ResourceManager registry ===\n";
        for (auto it = registry_.begin(); it != registry_.end(); ) {
            auto wp = it->second;
            if (auto sp = wp.lock()) {
                std::cout << " - " << it->first << " (alive, refcount=" << sp->refCount.load() << ")\n";
                ++it;
            } else {
                // expired -> cleanup
                it = registry_.erase(it);
            }
        }
        std::cout << "================================\n";
    }

private:
    ResourceManager() = default;
    ~ResourceManager() = default;
    std::mutex mutex_;
    std::unordered_map<std::string, std::weak_ptr<Resource>> registry_;
};

// ---------------------------------------------------
// SoftAssetPtr: stores ID only, resolves on demand
// ---------------------------------------------------
template<typename T>
class SoftAssetPtr
{
public:
    SoftAssetPtr() = default;
    SoftAssetPtr(std::string id) : id_(std::move(id)) {}
    // Resolve synchronously (load if needed) using provided loader
    // loader: std::function<std::shared_ptr<Resource>()> or lambda capturing id
    template<typename LoaderFunc>
    AssetHandle<T> ResolveSync(LoaderFunc loader) const {
        return ResourceManager::Get().LoadSync<T>(id_, loader);
    }

    // Resolve asynchronously
    template<typename LoaderFunc>
    std::future<AssetHandle<T>> ResolveAsync(LoaderFunc loader) const {
        return ResourceManager::Get().LoadAsync<T>(id_, loader);
    }

    const std::string& id() const { return id_; }

    // Try to get if already loaded
    AssetHandle<T> TryResolveExisting() const {
        return ResourceManager::Get().TryResolve<T>(id_);
    }

private:
    std::string id_;
};

// ---------------------------------------------------
// Example loader helpers (simulate disk/network load)
// In real engine these would read files, decode, create GPU resources etc.
// ---------------------------------------------------
std::shared_ptr<Resource> SimulateTextureLoad(const std::string& id, int w, int h, int delayMs = 200) {
    // simulate IO / decoding
    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    auto tex = std::make_shared<TextureResource>(id, w, h);
    return tex;
}

// ---------------------------------------------------
// Demo usage
// ---------------------------------------------------
int main() {
    std::cout << "=== Asset/Resource Control Demo ===\n";

    // Create soft pointer (like in asset references stored in maps/components)
    SoftAssetPtr<TextureResource> softTex("textures/hero_diffuse");

    // Try resolve existing -> none
    auto tryExisting = softTex.TryResolveExisting();
    std::cout << "Try resolve existing valid? " << tryExisting.isValid() << "\n";

    // Sync load (typical hot path)
    auto handle = softTex.ResolveSync([&]() -> std::shared_ptr<Resource> {
        return SimulateTextureLoad(softTex.id(), 1024, 1024, /*delayMs*/150);
    });
    if (handle.isValid()) {
        handle->debugInfo();
    }

    ResourceManager::Get().DebugPrintRegistry();

    {
        // Acquire another handle -> shared ownership
        auto handle2 = ResourceManager::Get().TryResolve<TextureResource>("textures/hero_diffuse");
        std::cout << "handle2 valid? " << handle2.isValid() << "\n";
        ResourceManager::Get().DebugPrintRegistry();
        // handle2 goes out of scope here (refCount decreases)
    }

    // Async load different asset
    SoftAssetPtr<TextureResource> softTex2("textures/env_sky");
    auto fut = softTex2.ResolveAsync([&]() -> std::shared_ptr<Resource> {
        return SimulateTextureLoad(softTex2.id(), 512, 256, /*delayMs*/400);
    });

    std::cout << "Doing other work while async loads...\n";
    // simulate other work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // get result
    auto asyncHandle = fut.get();
    if (asyncHandle.isValid()) asyncHandle->debugInfo();

    ResourceManager::Get().DebugPrintRegistry();

    // Manual unregister (hint to manager to forget about it)
    ResourceManager::Get().Unregister("textures/env_sky");
    ResourceManager::Get().DebugPrintRegistry();

    // Release all handles by resetting
    handle.reset();
    asyncHandle.reset();

    // After reset, cleanup occurs when shared_ptrs reach 0
    std::cout << "After releasing handles:\n";
    ResourceManager::Get().DebugPrintRegistry();

    std::cout << "=== End Demo ===\n";
    return 0;
}
