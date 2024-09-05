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
typedef DiscreteSize StoresAt;

// for control
ECS_STRUCT(Player, {
        int32_t up_key;
        int32_t down_key;
});

ECS_STRUCT_DECLARE(StoresAt, {
        int8_t x;
        int8_t y;
});

ECS_STRUCT_DECLARE(DiscreteSize, {
        int8_t x;
        int8_t y;
});

ECS_STRUCT_DECLARE(Position, {
        float x;
        float y;
});

ECS_TAG_DECLARE(Grabbable);
ECS_TAG_DECLARE(Grabbing);
ECS_TAG_DECLARE(Dropped);
ECS_TAG_DECLARE(Item);


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
        // do on a per player basis for multiplayer
        Vector2 delta = GetMouseDelta();

        for (int i = 0; i < it->count; i++) {
                int32_t index = 0;
                ecs_entity_t e;
                while ((e = ecs_get_target(it->world, it->entities[i], Grabbing, index++))) {
                        if (!ecs_has_id(it->world, e, ecs_id(Position))) {
                                continue;
                        }

                        Position *p = ecs_get_id(it->world, e, ecs_id(Position));
                        DiscreteSize *s = ecs_get_id(it->world, e, ecs_id(DiscreteSize));

                        p->x += delta.x;
                        p->y += delta.y;

                        if (IsMouseButtonUp(MOUSE_BUTTON_LEFT)) {
                                //end grab
                                ecs_remove_pair(it->world, it->entities[i], Grabbing, e);
                                ecs_add_id(it->world, e, Dropped);
                        }

                        if (IsKeyPressed(KEY_R)) {
                                s->x ^= s->y;
                                s->y ^= s->x;
                                s->x ^= s->y;
                        }
                }
        }
}

static void HandelDrop(ecs_iter_t *it) {
        Position *p = ecs_field(it, Position, 0);
        DiscreteSize *s = ecs_field(it, DiscreteSize, 1);

        for (int i = 0; i < it->count; i++) {

                bool found = false;
                ecs_iter_t menu_it = ecs_query_iter(it->world, it->ctx);
                while (ecs_query_next(&menu_it)) {
                        for (int j = 0; !found && j < menu_it.count; j++) {
                                Position *menu_p = ecs_field(&menu_it, Position, 0);
                                DiscreteSize *menu_s = ecs_field(&menu_it, DiscreteSize, 1);

                                Rectangle r = {menu_p[j].x, menu_p[j].y, menu_s[j].x * 20 - 1, menu_s[j].y * 20 - 1};

                                if (CheckCollisionPointRec(p[i], r)) {
                                        found = true;
                                        DiscretePosition tp = {((int32_t) (p[i].x - menu_p[j].x)) / 20, ((int32_t) (p[i].y - menu_p[j].y)) / 20};

                                        bool collide = false;
                                        ecs_entity_t first = 0;
                                        ecs_entity_t second = 0;

                                        ecs_type_t *type = ecs_get_type(it->world, menu_it.entities[j]);

                                        ecs_entity_t e;
                                        StoresAt *inv_loc = 0;

                                        for (int k = 0; !collide && k < type->count; k++) {
                                                ecs_id_t id = type->array[k];
                                                if (!ECS_IS_PAIR(id)) {
                                                        continue;
                                                }

                                                ecs_entity_t first = ecs_pair_first(it->world, id);
                                                ecs_entity_t second = ecs_pair_second(it->world, id);

                                                if (!ecs_has_id(it->world, second, Item)) {
                                                        continue;
                                                }

                                                StoresAt *inv_loc = ecs_get_pair(it->world, menu_it.entities[j], StoresAt, second);

                                                if (!inv_loc) {
                                                        continue;
                                                }

                                                //DiscreteSize *s1 = ecs_get(it->world, it->entities[i], DiscreteSize);
                                                const DiscreteSize *s2 = ecs_get_id(it->world, second, ecs_id(DiscreteSize));


                                                collide = second != it->entities[i]
                                                        && CheckCollisionRecs(
                                                                (Rectangle){tp.x, tp.y, s->y, s->x},
                                                                (Rectangle){inv_loc->x, inv_loc->y, s2->y, s2->x});
                                                        // add check for inv overflow
                                        }
                                        // get all items in said inventory
                                        // test against all items in inventory
                                        if (!collide) {
                                                p[i].x = tp.x * 20 + menu_p[j].x + 10;
                                                p[i].y = tp.y * 20 + menu_p[j].y + 10;

                                                ecs_set_pair(it->world, menu_it.entities[j], StoresAt, it->entities[i], {tp.x, tp.y});
                                        } else {
                                                const StoresAt *inv_loc = ecs_get_pair(it->world, menu_it.entities[j], StoresAt, second);
                                                if (inv_loc) {
                                                        p[i].x = inv_loc->x * 20 + menu_p[j].x + 10;
                                                        p[i].y = inv_loc->y * 20 + menu_p[j].y + 10;
                                                }
                                                else {
                                                        printf("collide\n");
                                                        // update to old pos
                                                }
                                        }
                                } else {
                                        ecs_query_t *inv_has_item = ecs_query(it->world, {
                                                .terms[0] = ecs_pair(ecs_id(StoresAt), it->entities[i])
                                        });

                                        ecs_iter_t it_inv = ecs_query_iter(it->world, inv_has_item);

                                        while (ecs_query_next(&it_inv)) {
                                                for (int l = 0; l < it_inv.count; l++) {
                                                        ecs_remove_pair(it->world, it_inv.entities[l], ecs_id(StoresAt), it->entities[i]);
                                                }
                                        }

                                        // ecs_pair_t

                                        ecs_query_fini(inv_has_item);
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
                Color color = RED;
                if (ecs_has_id(it->world, it->entities[i], Grabbable)){
                        color = GREEN;
                }


                for (int c = 0; c < s[i].x; c++) {
                        for (int r = 0; r < s[i].y; r++) {
                                DrawCircle(p[i].x + 20 * r, p[i].y + 20 * c, 10, color);
                        }
                }
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
        ECS_COMPONENT_DEFINE(world, StoresAt);
        ECS_COMPONENT_DEFINE(world, DiscreteSize);

        // ECS_COMPONENT(world, DiscreteSize);
        // ecs_struct(world,
        // {       .entity = ecs_id(DiscreteSize),
        //         .members = {
        //                 { .name = "w", .type = ecs_id(ecs_i8_t) },
        //                 { .name = "h", .type = ecs_id(ecs_i8_t) }
        //         }
        // });

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
        ECS_TAG_DEFINE(world, Item);
        ECS_TAG(world, Menu);

        ECS_TAG_DEFINE(world, Grabbable);
        ECS_TAG_DEFINE(world, Grabbing);
        ECS_TAG_DEFINE(world, Dropped);

        // game objects
        {
                ecs_entity_t e = ecs_entity(world, { .name = "obj.Inventory1"});
                ecs_set(world, e, Position, {200, 200});
                ecs_set(world, e, DiscreteSize, {9, 9});
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
                ecs_set(world, e, DiscreteSize, {2, 3});
                ecs_set(world, e, Position, {100, 100});
                ecs_add_id(world, e, Grabbable);
                ecs_add_id(world, e, Item);
        }

                {
                ecs_entity_t e = ecs_entity(world, { .name = "obj.Item.copy"});
                ecs_set(world, e, DiscreteSize, {3, 1});
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
                        {       .name = "HandelDrop",
                                .add = ecs_ids(ecs_dependson(EcsOnUpdate), Grabbable, Dropped, ecs_id(Position), ecs_id(DiscreteSize))
                        }),
                        .query.expr = "Position, DiscreteSize, [none] Item, [none] Dropped",
                        .callback = HandelDrop,
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
