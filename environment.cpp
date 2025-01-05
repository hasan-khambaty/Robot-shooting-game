#define GL_SILENCE_DEPRECATION
#include <glut/glut.h>
#include <math.h>
#include <stdio.h>
#include <cmath>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define M_PI 3.14159265358979323846

// Camera position
float cameraX = 0.0f, cameraY = 1.0f, cameraZ = 30.0f;
// Camera direction
float cameraAngle = 0.0f;

// Moving objects' positions
float obj1X = -2.0f, obj1Y = 1.0f, obj1Z = -5.0f;
float obj2X = 2.0f, obj2Y = 1.0f, obj2Z = -10.0f;
bool bulletFired = false;
float bulletX = 0.0f, bulletY = 2.0f, bulletZ = cameraZ;
float bulletSpeed = 0.4f;
int lastMouseX;
bool robot1Destroyed = false;
bool robot2Destroyed = false;
float robotSpeed = 0.10f;  // Adjust this value to control the speed of the robots
bool robot1BulletFired = false;
bool robot2BulletFired = false;
float robot1BulletX = 0.0f, robot1BulletY = 2.0f, robot1BulletZ = 0.0f;
float robot2BulletX = 0.0f, robot2BulletY = 2.0f, robot2BulletZ = 0.0f;
float robotBulletSpeed = 0.7f; // Speed of the robot's bullet
float robot1BulletDirX = 0.0f, robot1BulletDirY = 0.0f, robot1BulletDirZ = 0.0f;
float robot2BulletDirX = 0.0f, robot2BulletDirY = 0.0f, robot2BulletDirZ = 0.0f;
bool explosionActive = false;
float explosionX = 0.0f, explosionY = 0.0f, explosionZ = 0.0f;
float explosionSize = 0.4f;  // Initial size of the explosion
float minExplosionSize = 0.4f, maxExplosionSize = 0.8f;
clock_t explosionStartTime;
GLUquadric* quadric = gluNewQuadric();
GLuint robotTexture, cannonTexture;

GLuint loadTexture(const char* filename) {
    GLuint texture;
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);

    if (!image) {
        printf("Failed to load texture: %s\n", filename);
        return 0;
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(image);
    return texture;
}

void initTextures() {
    robotTexture = loadTexture("ironman.png");
    cannonTexture = loadTexture("cannontexture.jpeg");
}



void drawExplosionSphere(float radius, int slices, int stacks) {
    glPushMatrix();
    GLUquadricObj *quadric = gluNewQuadric();
    gluQuadricNormals(quadric, GLU_SMOOTH); // For smooth shading
    glColor3f(1.0f, 0.5f, 0.0f);  // Explosion color (orange)
    gluSphere(quadric, radius, slices, stacks);  // Draw the sphere
    gluDeleteQuadric(quadric);  // Clean up
    glPopMatrix();
}

void drawExplosion() {
    if (explosionActive) {
        float timeElapsed = float(clock() - explosionStartTime) / CLOCKS_PER_SEC;

        if (timeElapsed > 0.7f) {
            explosionActive = false;
            return;
        }

        // Gradually increase the explosion size from 0.4 to 0.8 over 0.7 seconds
        explosionSize = minExplosionSize + (maxExplosionSize - minExplosionSize) * (timeElapsed / 0.7f);

        // Draw the explosion (orange color)
        glColor3f(1.0f, 0.5f, 0.0f);  // Orange color
        glPushMatrix();
        glTranslatef(explosionX, explosionY, explosionZ);
        drawExplosionSphere(explosionSize, 20, 20);
        glPopMatrix();
    }
}

// Function to draw a cylinder for the tree trunk
void drawCylinder(float radius, float height, float r, float g, float b) {
    glColor3f(r, g, b);
    const int slices = 20;
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= slices; ++i) {
        float angle = 2.0f * M_PI * i / slices;
        float x = radius * cos(angle);
        float z = radius * sin(angle);
        glVertex3f(x, 0.0f, z);
        glVertex3f(x, height, z);
    }
    glEnd();
}

// Function to render the ground
void drawGround() {
    glClearColor(0.761f, 0.698f, 0.502f, 1.0f); // Grass color
    glBegin(GL_QUADS);
    glVertex3f(-50.0f, 0.0f, -50.0f);
    glVertex3f(50.0f, 0.0f, -50.0f);
    glVertex3f(50.0f, 0.0f, 50.0f);
    glVertex3f(-50.0f, 0.0f, 50.0f);
    glEnd();
}

// Joint angles
float rightHipAngle = 0.0f;
float rightKneeAngle = 0.0f;
float leftHipAngle = 0.0f;
float leftKneeAngle = 0.0f;
float bodyAngle = 0.0f;
float cannonAngle = 0.0f;
float LeftarmAngle = 140.0f;
float RightarmAngle = 60.0f;

// Control increments for stepping forward
float rightHipIncrement = 0.5f;
float rightKneeIncrement = 0.5f;
float leftHipIncrement = 0.5f;
float leftKneeIncrement = 0.5f;
bool isWalking = false;
bool isCannonSpinning = false; // Flag for cannon spinning state

// Currently selected joint for control
enum Joint { NONE, HIP, KNEE, BODY };
Joint selectedJoint = NONE;

// Color definitions (Red, Gold)
GLfloat red[] = {1.0f, 0.0f, 0.0f}; // Red for torso and upper arms
GLfloat gold[] = {1.0f, 0.84f, 0.0f}; // Gold for lower arms and head
GLfloat gray[] = {0.5f, 0.5f, 0.5f}; // Gray for cannon
GLfloat cyan[] = {0.0f, 1.0f, 1.0f};

// Draw a spherical joint with a gradient between red and yellow
void drawJoint() {
    glPushMatrix();
    glColor3f(1.0f, 0.5f, 0.0f); // Gradient color (between red and yellow)
    GLUquadricObj* quadObj = gluNewQuadric();
    gluQuadricDrawStyle(quadObj, GLU_FILL);
    gluSphere(quadObj, 0.15f, 16, 16); // Increased the size of the joint
    gluDeleteQuadric(quadObj);
    glPopMatrix();
}

void drawCylinder(float base, float top, float height, float red, float green, float blue)
{
    glColor3f(red, green, blue);
    GLUquadricObj *quadObj = gluNewQuadric();
    gluQuadricNormals(quadObj, GLU_SMOOTH);
    gluCylinder(quadObj, base, top, height, 20, 20);
    gluDeleteQuadric(quadObj);
}

void drawSphere(float r, float red, float green, float blue)
{
    glColor3f(red, green, blue);
    GLUquadricObj *quadObj = gluNewQuadric();
    gluQuadricNormals(quadObj, GLU_SMOOTH);
    gluSphere(quadObj, r, 40, 40);
    gluDeleteQuadric(quadObj);
}

// Function to draw a single arm
void drawArm(bool isRight) {
    glPushMatrix();
        glTranslatef(-0.8, 0.8, 0.0);
        glRotatef(LeftarmAngle, 0.2, 0.0, 0.0);
        drawCylinder(0.2, 0.18, 1.25, 1.0, 0.0, 0.0);
    glPopMatrix();
    glPushMatrix();
        glTranslatef(0.8, 0.8, 0.0);
        glRotatef(RightarmAngle, 1.0, 0.0, 0.0);
        drawCylinder(0.2, 0.18, 1.25, 1.0, 0.0, 0.0);
    glPopMatrix();
}

// Function to draw the cannon
void drawCannon() {
    glPushMatrix();
    glColor3fv(gray);

    // Position the cannon at the end of the right arm
    glTranslatef(0.75f, -0.8f, 0.10f);
    glTranslatef(-0.05, 0.30, 0.0);
    glRotatef(cannonAngle, 0, 0.05, 1);
    drawCylinder(0.2, 0.1, 1.0, 0.1, 0.1, 0.1);
    glPopMatrix();
}

// Function to draw a single leg with hip and knee angles
void drawLeg(bool isRight) {
    glPushMatrix();

    // Position the legs apart horizontally
    float legOffset = isRight ? 0.5f : -0.5f; // Space out legs to avoid overlap
    glTranslatef(legOffset, -1.0f, 0.0f); // Position legs below the torso

    // Hip and knee angles remain the same
    float hipAngle = isRight ? rightHipAngle : leftHipAngle;
    float kneeAngle = isRight ? rightKneeAngle : leftKneeAngle;

    // Draw hip joint
    drawJoint(); // Add hip joint

    // Hip joint
    glTranslatef(0.0f, -0.5f, 0.0f); // Position at the hip
    glRotatef(hipAngle, 1.0f, 0.0f, 0.0f); // Rotate at the hip
    glTranslatef(0.0f, -0.5f, 0.0f); // Offset after rotating the hip

    // Draw upper leg
    glPushMatrix();
    glColor3fv(red);
    GLUquadricObj* quadric = gluNewQuadric();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(quadric, 0.2f, 0.2f, 1.0f, 32, 32); // Upper leg
    gluDeleteQuadric(quadric);
    glPopMatrix();

    // Knee joint
    glTranslatef(0.0f, -1.0f, 0.0f);
    drawJoint(); // Add knee joint
    glRotatef(kneeAngle, 1.0f, 0.0f, 0.0f); // Rotate at the knee joint
    glTranslatef(0.0f, -0.15f, 0.0f); // Offset after rotating the knee

    // Draw lower leg
    glPushMatrix();
    glColor3fv(gold);
    quadric = gluNewQuadric();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(quadric, 0.18f, 0.18f, 1.0f, 32, 32); // Lower leg
    gluDeleteQuadric(quadric);
    glPopMatrix();

    glPopMatrix();
}

// Function to draw the torso (body of the robot)
void drawTorso() {
    glPushMatrix();
    glColor3fv(red);
    glScalef(1.0f, 1.5f, 0.5f); // Scale the torso (taller in y-axis)
    GLUquadricObj* quadObj = gluNewQuadric();
    gluQuadricNormals(quadObj, GLU_SMOOTH);
    gluSphere(quadObj, 0.5f, 32, 32); // Replaced cube with sphere for torso (alternative)
    gluDeleteQuadric(quadObj);
    // Draw joint at the torso
    drawJoint(); // Add torso joint
    glPopMatrix();
    glRotatef(bodyAngle, 0.0, 1.0, 0.0);
}

// Function to draw the head (moved higher above torso)
void drawHead() {
    glPushMatrix();
    glColor3fv(gold);
    glTranslatef(0.0f, 1.3f, 0.0f); // Position head higher above torso
    GLUquadricObj* quadObj = gluNewQuadric();
    gluQuadricNormals(quadObj, GLU_SMOOTH);
    gluSphere(quadObj, 0.4f, 32, 32); // Sphere for the head
    gluDeleteQuadric(quadObj);
    glPopMatrix();
    glColor3fv(cyan);
    glPushMatrix();
    glTranslatef(0.15f, 1.3f, 0.35f); // Position right eye
    glutSolidSphere(0.1f, 16, 16); // Right eye (kept as glut for simplicity)
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-0.15f, 1.3f, 0.35f); // Position left eye
    glutSolidSphere(0.1f, 16, 16); // Left eye (kept as glut for simplicity)
    glPopMatrix();
}

// Main function to draw the robot (with all parts)
void drawRobot(float x, float y, float z) {
        glPushMatrix();
    
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, robotTexture);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    
        // Move robot to its designated position
        glTranslatef(x, y + 1.0f, z); // Adjust y-position to ensure it is above ground

        // Rotate the entire robot
        glRotatef(bodyAngle, 0.0f, 1.0f, 0.0f);

        // Draw torso
        glPushMatrix();
        glTranslatef(0.0f, 0.02f, 0.0f); // Raise torso above base position
        drawTorso();
        glPopMatrix();

        // Draw head
        drawHead();

        // Draw left arm
        glPushMatrix();
        glTranslatef(-0.8f, 0.8f, 0.0f);  // Position left arm
        glRotatef(LeftarmAngle, 1.0f, 0.0f, 0.0f);
        drawCylinder(0.2f, 0.18f, 1.25f, 1.0f, 0.0f, 0.0f); // Left arm
        drawJoint();
        glPopMatrix();

        // Draw right arm
        glPushMatrix();
        glTranslatef(0.8f, 0.8f, 0.0f); // Position right arm
        glRotatef(RightarmAngle, 1.0f, 0.0f, 0.0f);
        drawCylinder(0.2f, 0.18f, 1.25f, 1.0f, 0.0f, 0.0f); // Right arm
        drawJoint();
        glPopMatrix();

        // Draw cannon attached to right arm
        drawCannon();

        // Draw legs
        drawLeg(true);  // Right leg
        drawLeg(false); // Left leg

        glPopMatrix();
        glDisable(GL_TEXTURE_2D);
}


// Function to draw the smaller cannon (turret style)
void drawTurretCannon() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, cannonTexture);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glPushMatrix();

    // Position the cannon on top of the turret base
    glTranslatef(0.0f, 0.9f, cameraZ - 0.1f); // Position it above the base

    // Rotate the cannon as needed
    glRotatef(cannonAngle + 180.0f, 0, 1, 0);  // Rotate along y-axis for turret-like rotation

    // Draw the cannon (smaller dimensions)
    glTranslatef(0.0f, 0.0f, 0.1f);  // Adjust position for the cannon length
    drawCylinder(0.05f, 0.05f, 1.0f, 0.8f, 0.1f, 0.1f);  // Smaller cannon

    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}

// Animation function for walking motion (with alternating leg movement)
void animateWalk(int value) {
    if (isWalking) {
        // Update right leg motion (hip and knee)
        rightHipAngle += rightHipIncrement;
        rightKneeAngle -= rightKneeIncrement;

        // Update left leg motion (hip and knee)
        leftHipAngle -= leftHipIncrement;
        leftKneeAngle += leftKneeIncrement;

        // Reset angles for a complete cycle
        if (rightHipAngle >= 30.0f || rightKneeAngle <= -20.0f) {
            rightHipAngle = 0.0f;
            rightKneeAngle = 0.0f;
        }
        if (leftHipAngle <= -30.0f || leftKneeAngle >= 20.0f) {
            leftHipAngle = 0.0f;
            leftKneeAngle = 0.0f;
        }
    }

    // Rotate the cannon if it's spinning
    if (isCannonSpinning) {
        cannonAngle += 2.0f; // Adjust speed as necessary
        if (cannonAngle >= 360.0f) {
            cannonAngle -= 360.0f; // Keep the angle within 0-360 range
        }
    }

    glutPostRedisplay(); // Request display update
    glutTimerFunc(30, animateWalk, 0); // Call this function again after 30 ms
}

void startWalking() {
    isWalking = true; // This is now always true
    glutTimerFunc(30, animateWalk, 0); // Start the walking animation
}

// Function to draw a bullet
void drawBullet() {
    glColor3f(1.0f, 1.0f, 0.0f); // Bullet color
    glPushMatrix();
    glTranslatef(bulletX, bulletY, bulletZ);
    gluSphere(quadric, 0.1f, 10, 10); // Small sphere as the bullet
    glPopMatrix();
}

bool checkCollision(float objX, float objY, float objZ) {
    float distance = sqrt(pow(bulletX - objX, 2) + pow(bulletY - objY, 2) + pow(bulletZ - objZ, 2));
    return distance < 1.0f; // Arbitrary value to check proximity
}

void moveRobotBullets() {
    if (robot1BulletFired) {
        robot1BulletX += robot1BulletDirX * robotBulletSpeed;
        robot1BulletY += robot1BulletDirY * robotBulletSpeed;
        robot1BulletZ += robot1BulletDirZ * robotBulletSpeed;

        // Stop the bullet if it reaches the camera
        float length = sqrt(pow(cameraX - robot1BulletX, 2) +
                            pow(cameraY - robot1BulletY, 2) +
                            pow(cameraZ - robot1BulletZ, 2));
        if (length < 1.0f) {
            robot1BulletFired = false;
        }
    }

    if (robot2BulletFired) {
        robot2BulletX += robot2BulletDirX * robotBulletSpeed;
        robot2BulletY += robot2BulletDirY * robotBulletSpeed;
        robot2BulletZ += robot2BulletDirZ * robotBulletSpeed;

        // Stop the bullet if it reaches the camera
        float length = sqrt(pow(cameraX - robot2BulletX, 2) +
                            pow(cameraY - robot2BulletY, 2) +
                            pow(cameraZ - robot2BulletZ, 2));
        if (length < 1.0f) {
            robot2BulletFired = false;
        }
    }
}

// Updated drawRobotBullet function
void drawRobotBullet(float x, float y, float z) {
    glColor3f(1.0f, 0.0f, 0.0f); // Robot bullet color (Red)
    glPushMatrix();
    glTranslatef(x, y, z);

    // Create a GLUquadricObj for rendering the sphere
    GLUquadricObj* quadObj = gluNewQuadric();
    gluQuadricNormals(quadObj, GLU_SMOOTH);  // Optional, for smooth shading
    gluSphere(quadObj, 0.1f, 10, 10);  // Small sphere as the robot's bullet

    // Clean up
    gluDeleteQuadric(quadObj);

    glPopMatrix();
}


// Updated fireRobotBullets function
void fireRobotBullets() {
    if (!robot1BulletFired && !robot1Destroyed) {
        robot1BulletFired = true;
        robot1BulletX = obj1X + 0.8f; // Cannon position offset
        robot1BulletY = obj1Y + 0.6f;
        robot1BulletZ = obj1Z;

        // Calculate direction vector for robot 1 bullet
        float dirX = cameraX - robot1BulletX;
        float dirY = cameraY - robot1BulletY;
        float dirZ = cameraZ - robot1BulletZ;
        float length = sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
        robot1BulletDirX = dirX / length;
        robot1BulletDirY = dirY / length;
        robot1BulletDirZ = dirZ / length;
    }

    if (!robot2BulletFired && !robot2Destroyed) {
        robot2BulletFired = true;
        robot2BulletX = obj2X + 0.8f; // Cannon position offset
        robot2BulletY = obj2Y + 1.0f;
        robot2BulletZ = obj2Z;

        // Calculate direction vector for robot 2 bullet
        float dirX = cameraX - robot2BulletX;
        float dirY = cameraY - robot2BulletY;
        float dirZ = cameraZ - robot2BulletZ;
        float length = sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
        robot2BulletDirX = dirX / length;
        robot2BulletDirY = dirY / length;
        robot2BulletDirZ = dirZ / length;
    }
}

// Timer function to fire bullets every 0.5 seconds
void robotBulletTimer(int value) {
    fireRobotBullets();
    glutTimerFunc(50, robotBulletTimer, 0); // Set timer for the next bullet
}


void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Camera position
    gluLookAt(cameraX, cameraY, cameraZ,
              cameraX + sin(cameraAngle), cameraY, cameraZ - cos(cameraAngle),
              0.0f, 1.0f, 0.0f);

    // Draw ground
    drawGround();

    // Draw the fixed cannon
    drawTurretCannon();

    // Draw moving robots only if they are not destroyed
    if (!robot1Destroyed) {
        drawRobot(obj1X, obj1Y, obj1Z); // First robot
    }
    if (!robot2Destroyed) {
        drawRobot(obj2X, obj2Y, obj2Z); // Second robot
    }

    // Draw bullet if fired
    if (bulletFired) {
        drawBullet();
    }
    
    if (robot1BulletFired) {
           drawRobotBullet(robot1BulletX, robot1BulletY, robot1BulletZ);
       }
       if (robot2BulletFired) {
           drawRobotBullet(robot2BulletX, robot2BulletY, robot2BulletZ);
       }

    // Draw explosion if active
    drawExplosion();

    glutSwapBuffers();
}



void idle() {
    // Move objects (robots) toward the camera, but only if they are not destroyed
    if (!robot1Destroyed) {
        obj1Z += robotSpeed;  // Move the first robot
    }
    if (!robot2Destroyed) {
        obj2Z += robotSpeed;  // Move the second robot
    }

    // Reset objects if they pass the camera, only if they are not destroyed
    if (!robot1Destroyed && obj1Z > cameraZ) obj1Z = -10.0f;
    if (!robot2Destroyed && obj2Z > cameraZ) obj2Z = -15.0f;

    // Move bullet if fired
    if (bulletFired) {
        // Calculate the direction of the camera's facing (normalized direction)
        float bulletDirectionX = sin(cameraAngle);
        float bulletDirectionZ = -cos(cameraAngle);

        // Move the bullet along the camera's direction
        bulletX += bulletDirectionX * bulletSpeed;
        bulletZ += bulletDirectionZ * bulletSpeed;

        // Check if the bullet hit any robot
        if (!robot1Destroyed && checkCollision(obj1X, obj1Y, obj1Z)) {
            robot1Destroyed = true; // Mark the first robot as destroyed
            bulletFired = false;    // Reset bullet
            explosionActive = true; // Activate explosion
            explosionX = obj1X;    // Position explosion at robot1
            explosionY = obj1Y;
            explosionZ = obj1Z;
            explosionStartTime = clock(); // Start the explosion timer
        }

        if (!robot2Destroyed && checkCollision(obj2X, obj2Y, obj2Z)) {
            robot2Destroyed = true; // Mark the second robot as destroyed
            bulletFired = false;    // Reset bullet
            explosionActive = true; // Activate explosion
            explosionX = obj2X;    // Position explosion at robot2
            explosionY = obj2Y;
            explosionZ = obj2Z;
            explosionStartTime = clock(); // Start the explosion timer
        }

        // If the bullet goes beyond the scene, reset it
        if (bulletZ < cameraZ - 10.0f) {
            bulletFired = false;
        }
    }
    
    moveRobotBullets();
    fireRobotBullets();
    
    glutPostRedisplay();
}

void mouseMotion(int x, int y) {
    // Compute the angle change based on the mouse's X position
    float deltaX = (float)(x - lastMouseX); // Difference in mouse's X position
    
    // Update cannon angle, adjust speed as needed
    cannonAngle += deltaX * 0.2f;  // Adjust sensitivity here (e.g., 0.2f)

    // Keep the cannon angle within 0-360 degrees range
    if (cannonAngle >= 360.0f) {
        cannonAngle -= 360.0f;
    } else if (cannonAngle < 0.0f) {
        cannonAngle += 360.0f;
    }
    
    const float sensitivity = 0.005f;

    int dx = x - lastMouseX;
    cameraAngle -= dx * sensitivity;

    // Update the last mouse X position for the next frame
    lastMouseX = x;

    glutPostRedisplay();  // Request display update
}

// Mouse callback to capture initial position
void mouse(int button, int state, int x, int y) {
    if (state == GLUT_DOWN) {
        lastMouseX = x;
    }
}

// Initialization
void init() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.482f , 0.537f , 0.576f , 1.0f); // Sky blue background
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHT0);
    
    GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
    GLfloat light_ambient[] = { 0.2, 0.2, 0.2, 1.0 };
    GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };

    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    
  
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
}

// Reshape callback
void reshape(int width, int height) {
    if (height == 0) height = 1;
    float aspect = (float)width / (float)height;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, aspect, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}


void handleKeyInput(unsigned char key, int x, int y)
{
    switch (key) {
        case ' ': // Fire bullet
            if (!bulletFired) {
                bulletFired = true;
                bulletX = 0.0f;
                bulletY = 1.0f;
                bulletZ = cameraZ - 2.0f; // Start at the cannon's position
            }
            break;
            
        case 's': // Reset game when 'S' is pressed
            // Reset robot positions and states
            obj1X = -2.0f;
            obj1Y = 1.0f;
            obj1Z = -5.0f;
            obj2X = 2.0f;
            obj2Y = 1.0f;
            obj2Z = -10.0f;

            // Reset the bullets
            bulletFired = false;
            robot1BulletFired = false;
            robot2BulletFired = false;
            robot1Destroyed = false;
            robot2Destroyed = false;

            // Reset explosion
            explosionActive = false;

            // Reset walking and cannon states
            isWalking = false;
            isCannonSpinning = false;
            cannonAngle = 0.0f;

            // Reset robot parts' angles
            rightHipAngle = 0.0f;
            rightKneeAngle = 0.0f;
            leftHipAngle = 0.0f;
            leftKneeAngle = 0.0f;
            bodyAngle = 0.0f;

            // Restart the walking animation
            startWalking();

            glutPostRedisplay();  // Redraw the scene
            break;

        case 27: // ESC key to quit
            exit(0);
    }
    glutPostRedisplay();
}


int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("robot game");
    glEnable(GL_DEPTH_TEST);
    initTextures();
    startWalking();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(handleKeyInput);
    glutMouseFunc(mouse);
    glutTimerFunc(0, animateWalk, 0);
    glutMotionFunc(mouseMotion);
    glutTimerFunc(50, robotBulletTimer, 0);
    glutMainLoop();
    return 0;
}
