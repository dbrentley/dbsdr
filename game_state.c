//
// Created by dbrent on 2/23/21.
//

#include "game_state.h"

#include <stdlib.h>
#include <stdio.h>

game_state_t *game_state_init() {
    game_state_t *gs = malloc(sizeof(game_state_t));
    if (gs == NULL) {
        fprintf(stderr, "Could not create game state\n");
        exit(-1);
    }
    gs->window_state = malloc(sizeof(window_state_t));
    if (gs->window_state == NULL) {
        fprintf(stderr, "Could not create game state - window state\n");
        exit(-1);
    }
    gs->mouse_state = malloc(sizeof(mouse_state_t));
    if (gs->mouse_state == NULL) {
        fprintf(stderr, "Could not create game state - mouse position\n");
        exit(-1);
    }
    gs->sdr_state = malloc(sizeof(sdr_state_t));
    if (gs->sdr_state == NULL) {
        fprintf(stderr, "Could not create game state - sdr state\n");
        exit(-1);
    }

    //game_state->cursor = custom_cursor(window);
    gs->should_close = 0;
    gs->window_state->resizing = 0;
    gs->window_state->width = DEFAULT_WIDTH;
    gs->window_state->height = DEFAULT_HEIGHT;
    gs->window_state->aspect =
            (float) DEFAULT_WIDTH / (float) DEFAULT_HEIGHT;
    gs->window_state->aspect = 1.0f;
    gs->window_state->zoom = 500.0f;
    gs->sdr_state->frequency = DEFAULT_FREQUENCY;

    return gs;
}

void game_state_destroy(game_state_t *gs) {
    glfwDestroyCursor(game_state->cursor);
    free(gs->mouse_state);
    gs->mouse_state = NULL;
    free(gs->window_state);
    gs->window_state = NULL;
    free(gs->sdr_state);
    gs->sdr_state = NULL;
    free(gs);
    gs = NULL;
}