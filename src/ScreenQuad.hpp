#pragma once

#include <glad/glad.h>

class ScreenQuad
{
    public:
	    // Init VAO/VBO (Lazily initialized)
        static void init()
        {
            if (!initialized)
            {
                glGenVertexArrays(1, &quadVAO);
                glGenBuffers(1, &quadVBO);

                glBindVertexArray(quadVAO);
                glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

                // set the vertex attributes pointers
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
                // UV attribute
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

			    // Unbind VAO/VBO
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindVertexArray(0);

                initialized = true;
            }
        }

        // Draw fullscreen quad
        static void draw()
        {
            if (!initialized)
            {
                init();
            }

            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }

	    // Cleanup resources (to be called at program termination)
        static void destroy()
        {
            if (initialized)
            {
                glDeleteVertexArrays(1, &quadVAO);
                glDeleteBuffers(1, &quadVBO);
                quadVAO = 0;
                quadVBO = 0;
                initialized = false;
            }
        }

    private:
        static inline float quadVertices[] = {
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };

        static inline unsigned int quadVAO = 0;
        static inline unsigned int quadVBO = 0;
        static inline bool initialized = false;
};
