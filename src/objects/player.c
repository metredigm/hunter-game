#include "player.h"
#include "camera.h"

#include <tds/tds.h>

#include "../tds_game/game_input.h"
#include "../tds_game/game_msg.h"

struct tds_object_type obj_player_type = {
	.type_name = "obj_player",
	.default_sprite = "spr_player_idle_right",
	.default_params = 0,
	.default_params_size = 0,
	.data_size = sizeof(struct obj_player_data),
	.func_init = obj_player_init,
	.func_destroy = obj_player_destroy,
	.func_update = obj_player_update,
	.func_draw = obj_player_draw,
	.func_msg = obj_player_msg,
	.func_import = obj_player_import,
	.func_export = obj_player_export,
	.save = 1
};

void obj_player_init(struct tds_object* ptr) {
	struct obj_player_data* data = (struct obj_player_data*) ptr->object_data;

	if (tds_engine_get_object_by_type(tds_engine_global, "obj_camera")) {
		tds_logf(TDS_LOG_WARNING, "There is already a camera which exists. This player will not create one.\n");
	} else {
		struct tds_object* camera = tds_object_create(&obj_camera_type, ptr->hmgr, ptr->smgr, ptr->x, ptr->y, 0.0f, NULL);
		tds_object_msg(camera, ptr, TDS_GAME_MSG_CAMERA_TRACK, ptr);
	}

	ptr->cbox_width = 0.3f;
	ptr->cbox_height = 0.9f;

	data->direction = 1;
}

void obj_player_destroy(struct tds_object* ptr) {
}

void obj_player_update(struct tds_object* ptr) {
	/* We will not allow slower movement than normal, we will use the axis and implement a basic deadzone */
	struct obj_player_data* data = (struct obj_player_data*) ptr->object_data;

	int move_key_low = tds_key_map_get(tds_engine_global->key_map_handle, TDS_GAME_INPUT_MOVE_LEFT);
	int move_key_high = tds_key_map_get(tds_engine_global->key_map_handle, TDS_GAME_INPUT_MOVE_RIGHT);
	int move_axis = tds_key_map_get(tds_engine_global->key_map_handle, TDS_GAME_INPUT_AXIS_MOVEMENT);
	int move_jump = tds_key_map_get(tds_engine_global->key_map_handle, TDS_GAME_INPUT_JUMP);

	float movement_axis = tds_input_map_get_axis(tds_engine_global->input_map_handle, move_key_low, move_key_high, move_axis);

	if (movement_axis <= -HUNTER_PLAYER_MOVE_DEADZONE) {
		data->movement_direction = -1;
		data->direction = -1;
	} else if (movement_axis >= HUNTER_PLAYER_MOVE_DEADZONE) {
		data->movement_direction = 1;
		data->direction = 1;
	} else {
		data->movement_direction = 0;
	}

	/* We will first change the player's x and y speeds as necessary. */
	ptr->xspeed += data->movement_direction * HUNTER_PLAYER_MOVE_ACCEL;

	if (ptr->xspeed < -HUNTER_PLAYER_MOVE_MAXSPEED) {
		ptr->xspeed = -HUNTER_PLAYER_MOVE_MAXSPEED;
	}

	if (ptr->xspeed > HUNTER_PLAYER_MOVE_MAXSPEED) {
		ptr->xspeed = HUNTER_PLAYER_MOVE_MAXSPEED;
	}

	if (data->can_jump && tds_input_map_get_key_pressed(tds_engine_global->input_map_handle, move_jump, 0)) {
		ptr->yspeed += HUNTER_PLAYER_JUMP;
		data->can_jump = 0;
	} else {
		ptr->yspeed += HUNTER_PLAYER_GRAVITY;
	}

	if (!data->movement_direction) {
		ptr->xspeed /= HUNTER_PLAYER_MOVE_DECEL;
	}

	/* Movement is addressed in a very special way which allows it to be smooth: we only act on XY collisions if X and Y both fail. */
	int collision_x = 0, collision_y = 0, collision_xy = 0;

	/* We will offset the player's position to test collisions. */
	float orig_x = ptr->x, orig_y = ptr->y;

	ptr->x = orig_x + ptr->xspeed;
	ptr->y = orig_y;

	if (tds_world_get_overlap_fast(tds_engine_global->world_handle, ptr)) {
		collision_x = 1;
	}

	ptr->x = orig_x;
	ptr->y = orig_y + ptr->yspeed;

	if (tds_world_get_overlap_fast(tds_engine_global->world_handle, ptr)) {
		collision_y = 1;

		data->can_jump = (ptr->yspeed < 0.0f);
	} else {
		data->can_jump = 0;
	}

	ptr->x = orig_x + ptr->xspeed;
	ptr->y = orig_y + ptr->yspeed;

	if (tds_world_get_overlap_fast(tds_engine_global->world_handle, ptr)) {
		collision_xy = 1;
	}

	ptr->x = orig_x;
	ptr->y = orig_y;

	if (collision_x) {
		ptr->xspeed = 0.0f;
	}

	if (collision_y) {
		ptr->yspeed = 0.0f;
	}

	if (collision_xy && !collision_x && !collision_y) {
		ptr->xspeed = ptr->yspeed = 0.0f;
	}
}

void obj_player_draw(struct tds_object* ptr) {
	struct obj_player_data* data = (struct obj_player_data*) ptr->object_data;

	/* Animation state switches! */

	if (data->can_jump) {
		if (data->movement_direction) {
			if (data->direction > 0) {
				tds_object_set_sprite(ptr, tds_sprite_cache_get(tds_engine_global->sc_handle, "spr_player_walk_right"));
			} else {
				tds_object_set_sprite(ptr, tds_sprite_cache_get(tds_engine_global->sc_handle, "spr_player_walk_left"));
			}
		} else {
			if (data->direction > 0) {
				tds_object_set_sprite(ptr, tds_sprite_cache_get(tds_engine_global->sc_handle, "spr_player_idle_right"));
			} else {
				tds_object_set_sprite(ptr, tds_sprite_cache_get(tds_engine_global->sc_handle, "spr_player_idle_left"));
			}
		}
	} else {
		if (data->direction > 0) {
			tds_object_set_sprite(ptr, tds_sprite_cache_get(tds_engine_global->sc_handle, "spr_player_jump_right"));
		} else {
			tds_object_set_sprite(ptr, tds_sprite_cache_get(tds_engine_global->sc_handle, "spr_player_jump_left"));
		}
	}
}

void obj_player_msg(struct tds_object* ptr, struct tds_object* sender, int msg, void* param) {
}

void obj_player_import(struct tds_object* ptr, struct tds_object_param* param) {
}

struct tds_object_param* obj_player_export(struct tds_object* ptr) {
	return 0;
}
