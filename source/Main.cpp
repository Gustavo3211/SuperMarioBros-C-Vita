#include <cstdio>
#include <iostream>

#include <SDL2/SDL.h>

#include "Emulation/Controller.hpp"
#include "SMB/SMBEngine.hpp"
#include "Util/Video.hpp"

#include "Configuration.hpp"
#include "Constants.hpp"

uint8_t* romImage;
static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_Texture* texture;
static SDL_Texture* scanlineTexture;
static SMBEngine* smbEngine = nullptr;
static uint32_t renderBuffer[RENDER_WIDTH * RENDER_HEIGHT];
static SDL_GameController* controller = nullptr;

/**
 * Load the Super Mario Bros. ROM image.
 */
static bool loadRomImage()
{
    const std::string& romPath = Configuration::getRomFileName();
    FILE* file = fopen(romPath.c_str(), "rb");
    if (file == NULL)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ROM Error", 
            "Could not find game.nes at ux0:data/SMB/game.nes\n\nExpected ROM: Super Mario Bros. (JU) (PRG0) [!].nes", window);
        return false;
    }

    // Find the size of the file
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0L, SEEK_SET);

    // Read the entire file into a buffer
    romImage = new uint8_t[fileSize];
    fread(romImage, sizeof(uint8_t), fileSize, file);
    fclose(file);

    return true;
}

/**
 * SDL Audio callback function.
 */
static void audioCallback(void* userdata, uint8_t* buffer, int len)
{
    if (smbEngine != nullptr)
    {
        smbEngine->audioCallback(buffer, len);
    }
}

/**
 * Initialize libraries for use.
 */
static bool initialize()
{
    // Initialize SDL first to use message box if needed
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0)
    {
        std::cout << "SDL_Init() failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Load the configuration
    Configuration::initialize(CONFIG_FILE_NAME);

    // Load the SMB ROM image
    if (!loadRomImage())
    {
        return false;
    }

    // Create the window (Vita fixed resolution)
    window = SDL_CreateWindow(APP_TITLE,
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              960, 544,
                              0);
    if (window == nullptr)
    {
        return false;
    }

    // Setup the renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr)
    {
        return false;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, RENDER_WIDTH, RENDER_HEIGHT);
    if (texture == nullptr)
    {
        return false;
    }

    if (Configuration::getScanlinesEnabled())
    {
        scanlineTexture = generateScanlineTexture(renderer);
    }

    // Set up custom palette, if configured
    if (!Configuration::getPaletteFileName().empty())
    {
        const uint32_t* palette = loadPalette(Configuration::getPaletteFileName());
        if (palette)
        {
            paletteRGB = palette;
        }
    }

    if (Configuration::getAudioEnabled())
    {
        SDL_AudioSpec desiredSpec;
        desiredSpec.freq = Configuration::getAudioFrequency();
        desiredSpec.format = AUDIO_S16SYS;
        desiredSpec.channels = 1;
        desiredSpec.samples = 4096; // ~100ms buffer
        desiredSpec.callback = audioCallback;
        desiredSpec.userdata = NULL;

        SDL_AudioSpec obtainedSpec;
        if (SDL_OpenAudio(&desiredSpec, &obtainedSpec) < 0) {
             std::cout << "SDL_OpenAudio failed: " << SDL_GetError() << std::endl;
        }
        SDL_PauseAudio(0);
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    // Open first available controller
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            controller = SDL_GameControllerOpen(i);
            if (controller) break;
        }
    }

    return true;
}

/**
 * Shutdown libraries for exit.
 */
static void shutdown()
{
    if (controller) SDL_GameControllerClose(controller);
    SDL_CloseAudio();

    if (scanlineTexture) SDL_DestroyTexture(scanlineTexture);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    delete[] romImage;

    SDL_Quit();
}

static void mainLoop()
{
    SMBEngine engine(romImage);
    smbEngine = &engine;
    engine.reset();

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT) running = false;
        }

        Controller& controller1 = engine.getController1();
        
        if (controller) {
            bool jump = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A) || // Vita Cross
                        SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B) || // Vita Circle
                        SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
            bool run  = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X) || // Vita Square
                        SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
            
            controller1.setButtonState(BUTTON_A, jump);
            controller1.setButtonState(BUTTON_B, run);
            controller1.setButtonState(BUTTON_SELECT, SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_BACK));
            controller1.setButtonState(BUTTON_START, SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START));

            const int threshold = 16384;
            int16_t axisX = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
            int16_t axisY = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);
            
            controller1.setButtonState(BUTTON_UP, SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP) || (axisY < -threshold));
            controller1.setButtonState(BUTTON_DOWN, SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN) || (axisY > threshold));
            controller1.setButtonState(BUTTON_LEFT, SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT) || (axisX < -threshold));
            controller1.setButtonState(BUTTON_RIGHT, SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) || (axisX > threshold));
        }
 else {
            // Fallback to keyboard
            const Uint8* keys = SDL_GetKeyboardState(NULL);
            controller1.setButtonState(BUTTON_A, keys[SDL_SCANCODE_X]);
            controller1.setButtonState(BUTTON_B, keys[SDL_SCANCODE_Z]);
            controller1.setButtonState(BUTTON_SELECT, keys[SDL_SCANCODE_BACKSPACE]);
            controller1.setButtonState(BUTTON_START, keys[SDL_SCANCODE_RETURN]);
            controller1.setButtonState(BUTTON_UP, keys[SDL_SCANCODE_UP]);
            controller1.setButtonState(BUTTON_DOWN, keys[SDL_SCANCODE_DOWN]);
            controller1.setButtonState(BUTTON_LEFT, keys[SDL_SCANCODE_LEFT]);
            controller1.setButtonState(BUTTON_RIGHT, keys[SDL_SCANCODE_RIGHT]);
            if (keys[SDL_SCANCODE_ESCAPE]) running = false;
        }

        engine.update();
        engine.render(renderBuffer);

        SDL_UpdateTexture(texture, NULL, renderBuffer, sizeof(uint32_t) * RENDER_WIDTH);

        SDL_RenderClear(renderer);

        // 4:3 Pillarbox: 960x544 screen. 4:3 area is 725x544 centered.
        SDL_Rect dest = { 117, 0, 725, 544 };
        SDL_RenderCopy(renderer, texture, NULL, &dest);

        if (Configuration::getScanlinesEnabled() && scanlineTexture)
        {
            SDL_RenderCopy(renderer, scanlineTexture, NULL, &dest);
        }

        SDL_RenderPresent(renderer);
    }
}

int main(int argc, char** argv)
{
    if (!initialize())
    {
        return -1;
    }

    mainLoop();
    shutdown();

    return 0;
}
