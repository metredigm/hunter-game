#include "env.h"

#include <tds/engine.h>
#include <tds/overlay.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct tds_object_type obj_env_type = {
	.type_name = "env",
	.default_sprite = (const char*) 0,
	.data_size = sizeof (struct obj_env_data),
	.func_init = obj_env_init,
	.func_destroy = obj_env_destroy,
	.func_update = obj_env_update,
	.func_draw = obj_env_draw,
	.func_msg = (void*) 0,
	.save = 0
};

void obj_env_init(struct tds_object* ptr) {
	struct obj_env_data* data = (struct obj_env_data*) ptr->object_data;

	data->wname = tds_object_get_spart(ptr, HUNTER_ENV_INDEX_WNAME);
	data->wname_pos = 0;
	data->wname_alpha = 1.0f;
	data->wname_interval_timer = tds_clock_get_point();
	data->wname_wait_timer = tds_clock_get_point();
}

void obj_env_destroy(struct tds_object* ptr) {
}

void obj_env_update(struct tds_object* ptr) {
	struct obj_env_data* data = (struct obj_env_data*) ptr->object_data;

	if (data->wname && data->wname_pos == strlen(data->wname) - 1 && tds_clock_get_ms(data->wname_alpha_timer) >= HUNTER_ENV_WNAME_ALPHA_TIME) {
		data->wname_alpha -= HUNTER_ENV_WNAME_ALPHA_DECAY;
	}
}

void obj_env_draw(struct tds_object* ptr) {
	struct obj_env_data* data = (struct obj_env_data*) ptr->object_data;

	if (data->wname && tds_clock_get_ms(data->wname_wait_timer) >= HUNTER_ENV_WNAME_WAIT) {
		int len = strlen(data->wname);

		if (data->wname_pos < len - 1) {
			if (tds_clock_get_ms(data->wname_interval_timer) > HUNTER_ENV_WNAME_INTERVAL) {
				if (++data->wname_pos >= len - 1) {
					data->wname_alpha_timer = tds_clock_get_point();
				}

				data->wname_interval_timer = tds_clock_get_point();
			}
		}

		tds_overlay_set_color(tds_engine_global->overlay_handle, 1.0f, 1.0f, 1.0f, data->wname_alpha);
		tds_overlay_render_text(tds_engine_global->overlay_handle, -1.0f, 1.0f, 1.0f, 0.0f, 14.0f, data->wname, data->wname_pos + 1, TDS_OVERLAY_REL_SCREENSPACE | TDS_OVERLAY_HCENTER | TDS_OVERLAY_VCENTER);
	}
}