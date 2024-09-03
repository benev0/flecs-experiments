#include "game.h"
#include "flecs.h"
#include "raylib.h"
#include <sys/param.h>
#include <math.h>

typedef Vector2 Position;
typedef Vector2 Velocity;

typedef struct {
        int8_t x;
        int8_t y;
} DiscreteSize;

typedef DiscreteSize DiscretePosition;

// for control
ECS_STRUCT(Player, {
        int32_t up_key;
        int32_t down_key;
});


ECS_STRUCT_DECLARE(Position, {
        float x;
        float y;
});

ECS_TAG_DECLARE(Grabbable);
ECS_TAG_DECLARE(Grabbing);
ECS_TAG_DECLARE(Dropped);


static void CheckPause(ecs_iter_t *it)
{
        if (IsKeyPressed(KEY_P))
        {
                if (ecs_has_id(it->world, EcsOnUpdate, EcsDisabled)) {
                        ecs_remove_id(it->world, EcsOnUpdate, EcsDisabled);
                } else {
                        ecs_add_id(it->world, EcsOnUpdate, EcsDisabled);
                }
        }
}


// take in input information
static void InteractPlayer(ecs_iter_t *it)
{
        Position *p = ecs_field(it, Position, 0);
        const Player *player = ecs_field(it, Player, 1); //no manip


        for (int i=0; i < it->count; i++) {
                p[i] = GetMousePosition();

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        // attempt grab
                } else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                        // move item
                } else if (IsMouseButtonUp(MOUSE_BUTTON_LEFT) /*& still holding item*/) {
                        // release item and decide wether to drop or place in an inventory
                }
                // else nothing
        }
}

float square(float x)
{
        return x * x;
}


static void HandleGrab(ecs_iter_t *it)
{
        Position *player_p = ecs_field(it, Position, 0);
        const Player *player = ecs_field(it, Player, 1); //no manip

        for (int i = 0; i < it->count; i++) {

                if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                {
                        return;
                }

                ecs_iter_t item_it = ecs_query_iter(it->world, it->ctx);

                // while or if?
                bool found = false;
                while (ecs_query_next(&item_it)) {
                        Position *item_p = ecs_field(&item_it, Position, 0);
                        DiscreteSize *item_s = ecs_field(&item_it, DiscreteSize, 1);


                        for (int j = 0; !found && j < item_it.count; j++) {
                                if ((square(player_p[i].x - item_p[j].x) + square(player_p[i].y - item_p[j].y)) < 100) {
                                        if (ecs_has_id(it->world, item_it.entities[i], Grabbable))
                                                ecs_remove_id(it->world, item_it.entities[i], Grabbable);
                                        ecs_add_pair(it->world, it->entities[i], Grabbing, item_it.entities[i]);
                                        found = true;
                                        break;
                                }
                        }

                }
        }
}

static void HandelDrag(ecs_iter_t *it)
{
        Vector2 delta = GetMouseDelta();

        for (int i = 0; i < it->count; i++) {
                int32_t index = 0;
                ecs_entity_t e;
                while ((e = ecs_get_target(it->world, it->entities[i], Grabbing, index++))) {
                        if (!ecs_has_id(it->world, e, ecs_id(Position))) {
                                continue;
                        }
                        Position *p = ecs_get_id(it->world, e, ecs_id(Position));
                        p->x += delta.x;
                        p->y += delta.y;

                        if (IsMouseButtonUp(MOUSE_BUTTON_LEFT)){
                                //end grab
                                ecs_remove_pair(it->world, it->entities[i], Grabbing, e);
                                ecs_add_id(it->world, e, Dropped);
                        }
                }
        }
}

static void HandleDrop(ecs_iter_t *it) {
        Position *p = ecs_field(it, Position, 0);
        DiscreteSize *s = ecs_field(it, DiscreteSize, 1);

        for (int i = 0; i < it->count; i++) {

                bool found = false;
                ecs_iter_t menu_it = ecs_query_iter(it->world, it->ctx);
                while (ecs_query_next(&menu_it)) {
                        for (int j = 0; !found && j < menu_it.count; j++) {
                                Position *menu_p = ecs_field(&menu_it, Position, 0);
                                DiscreteSize *menu_s = ecs_field(&menu_it, DiscreteSize, 1);

                                Rectangle r = {menu_p[j].x, menu_p[j].y, menu_s[j].x * 20, menu_s[j].y * 20};

                                if (CheckCollisionPointRec(p[i], r)) {
                                        p[i].x = (((int32_t) (p[i].x - menu_p[j].x)) / (20)) * 20 + menu_p[j].x + 10;
                                        p[i].y = (((int32_t) (p[i].y - menu_p[j].y)) / (20)) * 20 + menu_p[j].y + 10;
                                        found = true;
                                }
                        }
                }
                ecs_remove_id(it->world, it->entities[i], Dropped);
                ecs_add_id(it->world, it->entities[i], Grabbable);
        }
}


static void RenderInventory(ecs_iter_t *it)
{
        const Position *p = ecs_field(it, Position, 0);
        const DiscreteSize *s = ecs_field(it, DiscreteSize, 1);

        for (int i=0; i < it->count; i++) {
                DrawRectangle(p[i].x, p[i].y, s[i].x * 20, s[i].y * 20, BLUE);
                for (int c = 0; c < s[i].x; c++) {
                        for (int r = 0; r < s[i].y; r++) {
                                DrawRectangle(p[i].x + c * 20 + 1, p[i].y + r * 20 + 1, 18, 18, DARKBLUE);
                        }
                }

        }

}

static void RenderItems(ecs_iter_t *it)
{
        const Position *p = ecs_field(it, Position, 0);
        const DiscreteSize *s = ecs_field(it, DiscreteSize, 1);

        for (int i=0; i < it->count; i++) {
                Color c = RED;
                if (ecs_has_id(it->world, it->entities[i], Grabbable)){
                        c = GREEN;
                }

                DrawCircle(p[i].x, p[i].y, 10, c);
        }
}

static void BeginRendering(ecs_iter_t *it)
{
        (void)it;
        BeginDrawing();
        ClearBackground(BLACK);
}

static void EndRendering(ecs_iter_t *it)
{
        (void)it;
        DrawFPS(WINDOW_WIDTH - 100, 20);
        EndDrawing();
}

void setup_game(ecs_world_t *world)
{
        // definitions
        ECS_COMPONENT_DEFINE(world, Position);

        ECS_COMPONENT(world, DiscreteSize);
        ecs_struct(world,
        {       .entity = ecs_id(DiscreteSize),
                .members = {
                        { .name = "w", .type = ecs_id(ecs_i8_t) },
                        { .name = "h", .type = ecs_id(ecs_i8_t) }
                }
        });

        ECS_COMPONENT(world, DiscretePosition);
        ecs_struct(world,
        {       .entity = ecs_id(DiscretePosition),
                .members = {
                        { .name = "x", .type = ecs_id(ecs_i8_t) },
                        { .name = "y", .type = ecs_id(ecs_i8_t) }
                }
        });

        ECS_META_COMPONENT(world, Player);

        // ECS_TAG(world, Ball);
        ECS_TAG(world, Item);
        ECS_TAG(world, Menu);

        ECS_TAG_DEFINE(world, Grabbable);
        ECS_TAG_DEFINE(world, Grabbing);
        ECS_TAG_DEFINE(world, Dropped);

        // game objects
        {
                ecs_entity_t e = ecs_entity(world, { .name = "obj.Inventory1"});
                ecs_set(world, e, Position, {200, 200});
                ecs_set(world, e, DiscreteSize, {5, 3});
                ecs_add_id(world, e, Menu);
        }

        {
                ecs_entity_t e = ecs_entity(world, { .name = "obj.Inventory2"});
                ecs_set(world, e, Position, {0, 0});
                ecs_set(world, e, DiscreteSize, {1, 1});
                ecs_add_id(world, e, Menu);
        }

        {
                ecs_entity_t e = ecs_entity(world, { .name = "obj.Item"});
                ecs_set(world, e, DiscreteSize, {1, 1});
                ecs_set(world, e, Position, {100, 100});
                ecs_add_id(world, e, Grabbable);
                ecs_add_id(world, e, Item);
        }

                {
                ecs_entity_t e = ecs_entity(world, { .name = "obj.Item.copy"});
                ecs_set(world, e, DiscreteSize, {1, 1});
                ecs_set(world, e, Position, {200, 100});
                ecs_add_id(world, e, Grabbable);
                ecs_add_id(world, e, Item);
        }

        {
                ecs_entity_t e = ecs_entity(world, { .name = "obj.Player" });
                ecs_set(world, e, Position, {0, 0});
                ecs_set(world, e, Player, { KEY_W, KEY_S });
        }

        // systems
        {
                // generate partial order
                ecs_entity_t PreRendering = ecs_new_w_id(world, EcsPhase);
                ecs_set_name(world, PreRendering, "PreRendering");

                ecs_entity_t OnRendering = ecs_new_w_id(world, EcsPhase);
                ecs_set_name(world, OnRendering, "OnRendering");

                ecs_entity_t PostRendering = ecs_new_w_id(world, EcsPhase);
                ecs_set_name(world, PostRendering, "PostRendering");

                ecs_add_pair(world, PreRendering, EcsDependsOn, EcsOnStore);
                ecs_add_pair(world, OnRendering, EcsDependsOn, PreRendering);
                ecs_add_pair(world, PostRendering, EcsDependsOn, OnRendering);

                ECS_SYSTEM(world, CheckPause, EcsPreUpdate, 0);
                ECS_SYSTEM(world, InteractPlayer, EcsOnUpdate, Position, [in] Player);

                ecs_system(world,
                {       .entity = ecs_entity(world,
                        {       .name = "HandleGrab",
                                .add = ecs_ids(ecs_dependson(EcsOnUpdate), Grabbable, Grabbing, ecs_id(Position))
                        }),
                        .query.expr = "[in] Position, Player",
                        .callback = HandleGrab,
                        .ctx = ecs_query(world, { .expr = "Position, DiscreteSize, [none] Item, [none] Grabbable" })
                });

                ecs_system(world,
                {       .entity = ecs_entity(world,
                        {       .name = "HandelDrag",
                                .add = ecs_ids(ecs_dependson(EcsOnUpdate), Grabbable, Grabbing)
                        }),
                        .query.expr = "[none] Player",
                        .callback = HandelDrag
                });

                ecs_system(world,
                {       .entity = ecs_entity(world,
                        {       .name = "HandleDrop",
                                .add = ecs_ids(ecs_dependson(EcsOnUpdate), Grabbable, Dropped, ecs_id(Position), ecs_id(DiscreteSize))
                        }),
                        .query.expr = "Position, DiscreteSize, [none] Item, [none] Dropped",
                        .callback = HandleDrop,
                        .ctx = ecs_query(world, { .expr = "[in] Position, [in] DiscreteSize, [none] Menu, *"} )
                });

                // rendering steps
                // TODO: add partial order for rendering steps

                ECS_SYSTEM(world, BeginRendering, PreRendering, 0);

                ECS_SYSTEM(world, RenderInventory, OnRendering, [in] Position, [in] DiscreteSize, [none] Menu);
                ECS_SYSTEM(world, RenderItems, OnRendering, [in] Position, [in] DiscreteSize, [none] Item);

                ECS_SYSTEM(world, EndRendering, PostRendering, 0);

        }
}
