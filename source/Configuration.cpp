#include <fstream>
#include <algorithm>
#include <cctype>

#include "Configuration.hpp"

/**
 * List of all supported configuration options.
 */
std::list<ConfigurationOption*> Configuration::configurationOptions = {
    &Configuration::audioEnabled,
    &Configuration::audioFrequency,
    &Configuration::frameRate,
    &Configuration::paletteFileName,
    &Configuration::renderScale,
    &Configuration::romFileName,
    &Configuration::scanlinesEnabled,
    &Configuration::vsyncEnabled,
    &Configuration::debugMode
};

/**
 * Whether audio is enabled or not.
 */
BasicConfigurationOption<bool> Configuration::audioEnabled(
    "audio - experimental - will cause issues.enabled", false
);

/**
 * Audio frequency, in Hz
 */
BasicConfigurationOption<int> Configuration::audioFrequency(
    "audio.frequency", 48000
);

/**
 * Frame rate (per second).
 */
BasicConfigurationOption<int> Configuration::frameRate(
    "game.frame_rate", 60
);

/**
 * The filename for a custom palette to use for rendering.
 */
BasicConfigurationOption<std::string> Configuration::paletteFileName(
    "video.palette_file", ""
);

/**
 * Scaling factor for rendering.
 */
BasicConfigurationOption<int> Configuration::renderScale(
    "video.scale", 2
);

/**
 * Filename for the SMB ROM image.
 */
BasicConfigurationOption<std::string> Configuration::romFileName(
    "game.rom_file", "ux0:data/SMB/game.nes"
);

/**
 * Whether scanlines are enabled or not.
 */
BasicConfigurationOption<bool> Configuration::scanlinesEnabled(
    "video.scanlines", false
);

/**
 * Whether vsync is enabled for video.
 */
BasicConfigurationOption<bool> Configuration::vsyncEnabled(
    "video.vsync", true
);

/**
 * Whether debug mode is enabled or not.
 */
BasicConfigurationOption<bool> Configuration::debugMode(
    "game.debug_mode", false
);

ConfigurationOption::ConfigurationOption(
    const std::string& path) :
    path(path)
{
}

const std::string& ConfigurationOption::getPath() const
{
    return path;
}

static inline std::string trim(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
    return s;
}

void Configuration::initialize(const std::string& fileName)
{
    std::ifstream configFile(fileName.c_str());

    if (!configFile.good())
    {
        // Generate default config file if it doesn't exist
        std::ofstream newConfigFile(fileName.c_str());
        if (newConfigFile.good())
        {
            newConfigFile << "[audio - experimental - will cause issues]" << std::endl;
            newConfigFile << "enabled = 0" << std::endl;
            newConfigFile << std::endl;
            newConfigFile << "[game]" << std::endl;
            newConfigFile << "rom_file = ux0:data/SMB/game.nes" << std::endl;
            newConfigFile << "debug_mode = 0" << std::endl;
            newConfigFile.close();
        }
        
        // Re-open to read defaults
        configFile.open(fileName.c_str());
    }

    if (configFile.good())
    {
        std::map<std::string, std::string> settings;
        std::string line, section;
        while (std::getline(configFile, line))
        {
            line = trim(line);
            if (line.empty() || line[0] == ';' || line[0] == '#') continue;
            if (line[0] == '[' && line.back() == ']') {
                section = line.substr(1, line.size() - 2);
            } else {
                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string key = trim(line.substr(0, pos));
                    std::string value = trim(line.substr(pos + 1));
                    settings[section + "." + key] = value;
                }
            }
        }

        for (auto option : configurationOptions)
        {
            option->initializeValue(settings);
        }
    }
}

bool Configuration::getAudioEnabled()
{
    return audioEnabled.getValue();
}

int Configuration::getAudioFrequency()
{
    return audioFrequency.getValue();
}

int Configuration::getFrameRate()
{
    return frameRate.getValue();
}

const std::string& Configuration::getPaletteFileName()
{
    return paletteFileName.getValue();
}

int Configuration::getRenderScale()
{
    return renderScale.getValue();
}

const std::string& Configuration::getRomFileName()
{
    return romFileName.getValue();
}

bool Configuration::getScanlinesEnabled()
{
    return scanlinesEnabled.getValue();
}

bool Configuration::getVsyncEnabled()
{
    return vsyncEnabled.getValue();
}

bool Configuration::getDebugMode()
{
    return debugMode.getValue();
}
