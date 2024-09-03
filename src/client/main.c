#include "raylib.h"
#include "flecs.h"
#include "game.h"

// https://github.com/pope/raylib-flecs-pong/blob/main/src/pong.c

int main(void)
{

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Menus Test");

    ecs_world_t *world = ecs_init();
    ecs_set_threads(world, 2);
    ecs_log_set_level (-1);
    ecs_log_enable_colors (true);
    ecs_set_target_fps (world, 60);

    ecs_singleton_set(world, EcsRest, { 0 });
    ECS_IMPORT(world, FlecsStats);

    setup_game(world);

    while (!WindowShouldClose() && ecs_progress(world, 0.0)) { }

    ecs_fini(world);
    CloseWindow();

    return 0;
}
