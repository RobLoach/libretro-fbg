#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "libretro.h"
//#include "libretro_core_options.h"

#include "fbg/src/fbgraphics.h"

static struct retro_log_callback logging;
static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

struct fbg_core
{
	bool quit;
	struct _fbg* fbg;
};
static struct fbg_core* core;

/**
 * TIC-80 callback; Report an error from the TIC-80 cart.
 */
void fbg_libretro_error(const char* info)
{
	// Report the error to the log.
	log_cb(RETRO_LOG_ERROR, "[fbg]: %s\n", info);

	// Display the error on the screen, if possible.
	if (environ_cb) {
		struct retro_message msg = {
			info,
			400
		};
		environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
	}

	// Finally, on an error, close the core.
}

/**
 * libretro callback; Handles the logging internally when the logging isn't set.
 */
void fbg_libretro_fallback_log(enum retro_log_level level, const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
}

/**
 * libretro callback; Global initialization.
 */
RETRO_API void retro_init(void)
{
	core = malloc(sizeof(struct fbg_core));
}

/**
 * libretro callback; Global deinitialization.
 */
RETRO_API void retro_deinit(void)
{
	// Make sure the game is unloaded.
	retro_unload_game();

	free(core);
}

/**
 * libretro callback; Retrieves the internal libretro API version.
 */
RETRO_API unsigned retro_api_version(void)
{
	return RETRO_API_VERSION;
}

/**
 * libretro callback; Reports device changes.
 */
RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device)
{
	log_cb(RETRO_LOG_INFO, "Plugging device %u into port %u.\n", device, port);
}

/**
 * libretro callback; Retrieves information about the core.
 */
RETRO_API void retro_get_system_info(struct retro_system_info *info)
{
	memset(info, 0, sizeof(*info));
	info->library_name     = "fbg";
	info->library_version  = "0.0.1";
	info->valid_extensions = "";
	info->need_fullpath    = false;
	info->block_extract    = false;
}

/**
 * libretro callback; Get information about the desired audio and video.
 */
RETRO_API void retro_get_system_av_info(struct retro_system_av_info *info)
{
	info->timing = (struct retro_system_timing) {
		.fps = 60,
		.sample_rate = 44000,
	};

	info->geometry = (struct retro_game_geometry) {
		.base_width   = 640,
		.base_height  = 480,
		.max_width    = 640,
		.max_height   = 480,
		.aspect_ratio = (float)640 / (float)480,
	};
}

/**
 * libretro callback; Sets up the environment callback.
 */
RETRO_API void retro_set_environment(retro_environment_t cb)
{
	// Update the environment callback to make environment calls.
	environ_cb = cb;

	bool no_content = true;
	cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

	// Set up the logging interface.
	if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging)) {
		log_cb = logging.log;
	}
	else {
		log_cb = fbg_libretro_fallback_log;
	}

	//libretro_set_core_options(environ_cb);
}

/**
 * libretro callback; Set up the audio sample callback.
 */
RETRO_API void retro_set_audio_sample(retro_audio_sample_t cb)
{
	audio_cb = cb;
}

/**
 * libretro callback; Set up the audio sample batch callback.
 *
 * @see tic80_libretro_audio()
 */
RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
	audio_batch_cb = cb;
}

/**
 * libretro callback; Set up the input poll callback.
 */
RETRO_API void retro_set_input_poll(retro_input_poll_t cb)
{
	input_poll_cb = cb;
}

/**
 * libretro callback; Set up the input state callback.
 */
RETRO_API void retro_set_input_state(retro_input_state_t cb)
{
	input_state_cb = cb;
}

/**
 * libretro callback; Set up the video refresh callback.
 */
RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb)
{
	video_cb = cb;
}

/**
 * libretro callback; Reset the game.
 */
RETRO_API void retro_reset(void)
{
	
}

/**
 * libretro callback; Load the labels for the input buttons.
 */
void fbg_libretro_input_descriptors()
{
	struct retro_input_descriptor desc[] = {
		// Player 1
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "A" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "B" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "Y" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "X" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "Slow Mouse" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "Mouse Right Click" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "Mouse Left Click" },
		// Player 2
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "A" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "B" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "Y" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "X" },
		// Player 3
		{ 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left" },
		{ 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up" },
		{ 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down" },
		{ 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
		{ 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "A" },
		{ 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "B" },
		{ 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "Y" },
		{ 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "X" },
		// Player 4
		{ 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left" },
		{ 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up" },
		{ 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down" },
		{ 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
		{ 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "A" },
		{ 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "B" },
		{ 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "Y" },
		{ 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "X" },

		{ 0 },
	};

	environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
}

/**
 * Update the input state, and tick the game.
 */
void fbg_libretro_update(struct fbg_core* game)
{
	// Let libretro know that we need updated input states.
	input_poll_cb();


	// Update the game state.
	//tic80_tick(game, &state->input);
	//tic80_sound(game);
}

/**
 * Draw the screen.
 */
void fbg_libretro_draw(struct _fbg* fbg)
{
    //video_cb(game->screen, TIC80_FULLWIDTH, TIC80_FULLHEIGHT, TIC80_FULLWIDTH << 2);

	video_cb(fbg->disp_buffer, fbg->width, fbg->height, fbg->width << 2);
}

/**
 * Play the TIC-80 audio.
 *
 * @see retro_run()
 */
void fbg_libretro_audio(struct fbg_core* game)
{
	// Tell libretro about the samples.
	//audio_batch_cb(game->sound.samples, game->sound.count / TIC_STEREO_CHANNELS);
}

/**
 * Update the state of the core variables.
 */
void fbg_libretro_variables(bool startup)
{
	// Check all the individual variables for the core.
	struct retro_variable var;

	// Gamepad Analog Deadzone
	// state->analogDeadzone = (int)(0.15f * (float)RETRO_ANALOG_RANGE);
	// var.key = "tic80_analog_deadzone";
	// var.value = NULL;
	// if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
	// 	state->analogDeadzone = (int)((float)atoi(var.value) * 0.01f * (float)RETRO_ANALOG_RANGE);
	// }
}

/**
 * libretro callback; Render the screen and play the audio.
 */
RETRO_API void retro_run(void)
{
	// Ensure the state is set up.
	if (core == NULL) {
		return;
	}

	if (core->fbg) {

		fbg_rect(core->fbg, 0, 0, 640, 480, 255, 0, 0);

		fbg_libretro_draw(core->fbg);
	}
}

/**
 * libretro callback; Load a game.
 */
RETRO_API bool retro_load_game(const struct retro_game_info *info)
{
	// Initialize the core if it hasn't been yet.
	//if (state == NULL) {
	//	retro_init();
	//}

	// Pixel format.
	enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
	if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
		log_cb(RETRO_LOG_ERROR, "RETRO_PIXEL_FORMAT_XRGB8888 is not supported.\n");
		return false;
	}

	// Check for the content.
	/*
	if (info == NULL) {
		log_cb(RETRO_LOG_ERROR, "No content information provided.\n");
		return false;
	}

	// Ensure content data is available.
	if (info->data == NULL) {
		log_cb(RETRO_LOG_ERROR, "No content data provided.\n");
		return false;
	}
	*/

	core->fbg = fbg_customSetup(640, 480, 4, 1, 0, core, NULL, NULL, NULL, NULL);

	// Set up the input descriptors.
	fbg_libretro_input_descriptors();

	// Load up any core variables.
	fbg_libretro_variables(true);

	return true;
}

/**
 * libretro callback; Tells the core to unload the game.
 */
RETRO_API void retro_unload_game(void)
{
	if (core->fbg) {
		fbg_close(core->fbg);
		core->fbg = NULL;
	}
}

/**
 * libretro callback; Retrieves the region for the content.
 */
RETRO_API unsigned retro_get_region(void)
{
	return RETRO_REGION_NTSC;
}

/**
 * libretro callback; Load a game using a subsystem.
 */
RETRO_API bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
	// Forward subsystem requests over to retro_load_game().
	return retro_load_game(info);
}

/**
 * libretro callback; Retrieve the size of the serialized memory.
 */
size_t retro_serialize_size(void)
{
	return 0;
}

/**
 * libretro callback; Get the current persistent memory.
 */
RETRO_API bool retro_serialize(void *data, size_t size)
{
	return false;
}

/**
 * libretro callback; Given the serialized data, load it into the persistent memory.
 */
RETRO_API bool retro_unserialize(const void *data, size_t size)
{
	return false;
}

/**
 * libretro callback; Gets region of memory.
 */
RETRO_API void *retro_get_memory_data(unsigned id)
{
	return NULL;
}

/**
 * libretro callback; Gets the size of the given memory slot.
 */
RETRO_API size_t retro_get_memory_size(unsigned id)
{
	return 0;
}

/**
 * libretro callback; Reset all cheats to disabled.
 */
RETRO_API void retro_cheat_reset(void)
{
	// Nothing.
}

/**
 * libretro callback; Enable/disable the given cheat code.
 */
RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
    
}
