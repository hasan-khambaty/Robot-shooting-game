This project implements a texture-mapped robot and turret cannon using OpenGL and the stb_image.h library for loading textures. The robot and turret cannon models are rendered with textures applied to their surfaces, controlled through a simple GLUT-based window.

Features

Texture Mapping: Textures are applied to both the robot and turret cannon using stb_image.h.

Key Press Interaction:

Press S to restart the game and reset the robot and cannon positions. Press spacebar to shoot the bullets from the cannon The cannon will move from the mouse around the screen

OpenGL Rendering: The models are rendered using OpenGL primitives with lighting and depth testing enabled.

Prerequisites

Ensure you have the following installed:

C++ Compiler (e.g., g++)

OpenGL libraries (freeglut, GL, GLU)

File Structure

main.cpp: Contains the main source code for rendering and handling input.

stb_image.h: Image loading library used for texture loading.

ironman.png: Texture for the robot model.

cannontexture.jpeg: Texture for the turret cannon.

How to Run

Clone the repository:

git clone https://github.com/hasan-khambaty/Robot-shooting-game

cd Robot-shooting-game

Compile the program:

g++ main.cpp -lGL -lGLU -lglut -o texture_mapping

Run the executable:

./texture_mapping
