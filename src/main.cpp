/*******************************************************************************************
 *
 * raylib [core] example - 3D Free Kick Simulator
 *
 * This example has been created using raylib 5.0 (www.raylib.com)
 * raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
 *
 * Copyright (c) 2024 Ramon Santamaria (@raysan5)
 *
 ********************************************************************************************/

#include "raylib.h"
#include "raymath.h"
#include <iostream>
#include <string>

// Types and Structures Definition
struct Ball
{
    Vector3 position;
    Vector3 velocity;
    Vector3 acceleration;
    Vector3 angularVelocity; // For spin
    float radius;
    float mass;
    bool isKicked;
};

enum GameState
{
    STATE_AIMING,
    STATE_SIMULATING,
    STATE_RESET
};

// Magnus Force Func for Spin
Vector3 CalculateMagnusForce(const Ball &ball)
{
    // Simplified Magnus effect formula.
    // F_m = C * (w x v)
    // C is a constant that depends on air density, ball shape, etc.
    const float liftCoefficient = 0.05f;
    return Vector3Scale(Vector3CrossProduct(ball.angularVelocity, ball.velocity), liftCoefficient);
}

// Function for air res/ drag
Vector3 CalculateDragForce(const Ball &ball)
{
    // F_d = -k * v^2 * normalize(v)
    // drag formula.
    const float dragCoefficient = 0.01f;
    float speed = Vector3Length(ball.velocity);
    if (speed > 0)
    {
        Vector3 dragDirection = Vector3Normalize(Vector3Negate(ball.velocity));
        return Vector3Scale(dragDirection, dragCoefficient * speed * speed);
    }
    return Vector3Zero();
}

int main(void)
{
    // Initialization

    const int screenWidth = 1920;
    const int screenHeight = 1080;

    InitWindow(screenWidth, screenHeight, "3D Free Kick Simulator");

    // Camera
    Camera camera = {0};
    camera.position = {0.0f, 5.0f, -30.0f}; // Camera position
    camera.target = {0.0f, 2.0f, 0.0f};     // Camera looking at point
    camera.up = {0.0f, 1.0f, 0.0f};         // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                    // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE; // Camera mode type

    // Initialize Ball
    Ball ball = {0};
    ball.position = {0.0f, 0.5f, 0.0f};
    ball.velocity = {0.0f, 0.0f, 0.0f};
    ball.acceleration = {0.0f, 0.0f, 0.0f};
    ball.angularVelocity = {0.0f, 0.0f, 0.0f};
    ball.radius = 0.5f;
    ball.mass = 0.45f; // kg
    ball.isKicked = false;

    // Physics constants
    const Vector3 gravity = {0.0f, -9.8f, 0.0f}; // m/s^2

    // Goal properties
    const float goalWidth = 10.0f;
    const float goalHeight = 4.0f;
    const float goalDepth = 2.0f;
    const Vector3 goalPosition = {0.0f, goalHeight / 2.0f, 25.0f};
    BoundingBox goalBox = {
        {goalPosition.x - goalWidth / 2, goalPosition.y - goalHeight / 2, goalPosition.z - goalDepth / 2},
        {goalPosition.x + goalWidth / 2, goalPosition.y + goalHeight / 2, goalPosition.z + goalDepth / 2}};

    // Kick parameters
    float kickStrength = 50.0f; // Kick power
    float kickAngle = 15.0f;    // in degrees
    float kickSpin = 0.0f;      // radians/s around Y-axis

    GameState currentState = STATE_AIMING;
    std::string goalMessage = "";
    float messageTimer = 0.0f;

    SetTargetFPS(60);

    // main simulation loop
    while (!WindowShouldClose())
    {
        // Update
        float dt = GetFrameTime(); // delta time for physics calculation

        if (messageTimer > 0)
        {
            messageTimer -= dt;
        }
        else
        {
            goalMessage = "";
        }

        switch (currentState)
        {
        case STATE_AIMING:
        {
            // User Controls

            // Kick Power
            if (IsKeyDown(KEY_UP))
                kickStrength += 10.0f * dt;
            if (IsKeyDown(KEY_DOWN))
                kickStrength -= 10.0f * dt;
            if (kickStrength < 10.0f)
                kickStrength = 10.0f;
            if (kickStrength > 100.0f)
                kickStrength = 100.0f;

            // Kick Angle
            if (IsKeyDown(KEY_W))
                kickAngle += 10.0f * dt;
            if (IsKeyDown(KEY_S))
                kickAngle -= 10.0f * dt;
            if (kickAngle < 0.0f)
                kickAngle = 0.0f;
            if (kickAngle > 45.0f)
                kickAngle = 45.0f;

            // Spin
            if (IsKeyDown(KEY_RIGHT))
                kickSpin += 20.0f * dt;
            if (IsKeyDown(KEY_LEFT))
                kickSpin -= 20.0f * dt;
            if (kickSpin > 50.0f)
                kickSpin = 50.0f;
            if (kickSpin < -50.0f)
                kickSpin = -50.0f;

            // Kick Ball
            if (IsKeyPressed(KEY_SPACE))
            {
                ball.isKicked = true;

                // initial velocity vector from strength and angle
                float angleRad = DEG2RAD * kickAngle;
                ball.velocity.x = 0;
                ball.velocity.y = sinf(angleRad) * kickStrength;
                ball.velocity.z = cosf(angleRad) * kickStrength;

                // angular velocity for spin
                ball.angularVelocity = {0.0f, kickSpin, 0.0f};

                currentState = STATE_SIMULATING;
            }
        }
        break;

        case STATE_SIMULATING:
        {
            if (ball.isKicked)
            {
                // Physics Simulation

                // forces calculations
                Vector3 force_gravity = Vector3Scale(gravity, ball.mass);
                Vector3 force_magnus = CalculateMagnusForce(ball);
                Vector3 force_drag = CalculateDragForce(ball);

                // Sun of Forces
                Vector3 totalForce = Vector3Add(force_gravity, Vector3Add(force_magnus, force_drag));

                // Calculate acceleration (F=ma -> a=F/m)
                ball.acceleration = Vector3Scale(totalForce, 1.0f / ball.mass);

                // Update velocity (v = v0 + at)
                ball.velocity = Vector3Add(ball.velocity, Vector3Scale(ball.acceleration, dt));

                // Update position (p = p0 + vt)
                ball.position = Vector3Add(ball.position, Vector3Scale(ball.velocity, dt));

                // Collision Detection
                // Ground collision
                if (ball.position.y < ball.radius)
                {
                    ball.position.y = ball.radius;
                    ball.velocity.y *= -0.7f; // bounce

                    // friction to slow down ball rolling and spinning
                    ball.velocity.x *= 0.9f;
                    ball.velocity.z *= 0.9f;
                    ball.angularVelocity = Vector3Scale(ball.angularVelocity, 0.9f);
                }

                // Goal collision
                if (CheckCollisionBoxSphere(goalBox, ball.position, ball.radius))
                {
                    goalMessage = "GOAL!!!";
                    messageTimer = 3.0f;
                    currentState = STATE_RESET;
                }

                // out of bounds / stopped check
                if (Vector3Length(ball.velocity) < 0.1f && ball.position.y <= ball.radius + 0.01f)
                {
                    currentState = STATE_RESET;
                }
                if (fabs(ball.position.x) > 50.0f || ball.position.z > 50.0f)
                {
                    currentState = STATE_RESET;
                }
            }
        }
        break;

        case STATE_RESET:
        {

            if (IsKeyPressed(KEY_ENTER))
            {
                ball.position = {0.0f, 0.5f, 0.0f};
                ball.velocity = Vector3Zero();
                ball.acceleration = Vector3Zero();
                ball.angularVelocity = Vector3Zero();
                ball.isKicked = false;
                kickStrength = 50.0f;
                kickAngle = 15.0f;
                kickSpin = 0.0f;
                currentState = STATE_AIMING;
            }
        }
        break;
        }

        // Draw

        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        // ground
        DrawPlane((Vector3){0.0f, 0.0f, 0.0f}, (Vector2){100.0f, 100.0f}, LIME);
        DrawGrid(100, 1.0f);

        // Ball
        DrawSphere(ball.position, ball.radius, RED);
        DrawSphereWires(ball.position, ball.radius, 8, 8, DARKGRAY);

        // Goal
        DrawCubeWires(goalPosition, goalWidth, goalHeight, goalDepth, BLACK);

        DrawLine3D({goalPosition.x - goalWidth / 2, goalPosition.y - goalHeight / 2, goalPosition.z}, {goalPosition.x + goalWidth / 2, goalPosition.y - goalHeight / 2, goalPosition.z}, GRAY);
        DrawLine3D({goalPosition.x - goalWidth / 2, goalPosition.y + goalHeight / 2, goalPosition.z}, {goalPosition.x + goalWidth / 2, goalPosition.y + goalHeight / 2, goalPosition.z}, GRAY);

        EndMode3D();

        // UI
        DrawText("Free Kick Simulator", 10, 10, 20, DARKGRAY);

        if (currentState == STATE_AIMING)
        {
            DrawText("STATE: AIMING", 10, 40, 20, DARKGRAY);
            DrawText(TextFormat("Power (Up/Down): %.1f", kickStrength), 10, 70, 20, MAROON);
            DrawText(TextFormat("Angle (W/S): %.1f", kickAngle), 10, 100, 20, MAROON);
            DrawText(TextFormat("Spin (Left/Right): %.1f", kickSpin), 10, 130, 20, MAROON);
            DrawText("PRESS SPACE TO KICK", screenWidth / 2 - MeasureText("PRESS SPACE TO KICK", 30) / 2, screenHeight - 50, 30, GRAY);
        }
        else if (currentState == STATE_SIMULATING)
        {
            DrawText("STATE: SIMULATING", 10, 40, 20, DARKGRAY);
        }
        else if (currentState == STATE_RESET)
        {
            DrawText("PRESS ENTER TO RESET", screenWidth / 2 - MeasureText("PRESS ENTER TO RESET", 30) / 2, screenHeight - 50, 30, GRAY);
        }

        // Display Goal Message
        if (messageTimer > 0)
        {
            DrawText(goalMessage.c_str(), screenWidth / 2 - MeasureText(goalMessage.c_str(), 50) / 2, screenHeight / 2 - 25, 50, GOLD);
        }

        DrawFPS(screenWidth - 90, 10);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
