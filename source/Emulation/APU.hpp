#ifndef APU_HPP
#define APU_HPP

#include <cstdint>

#define AUDIO_BUFFER_LENGTH 16384

class Pulse;
class Triangle;
class Noise;

struct SampleState {
    uint8_t p1, p2, t, n;
};

/**
 * Audio processing unit emulator.
 */
class APU
{
public:
    APU();
    ~APU();

    /**
     * Step the APU by one frame.
     */
    void stepFrame();

    void output(uint8_t* buffer, int len);

    void writeRegister(uint16_t address, uint8_t value);

    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }

private:
    bool enabled;
    int frameValue; /**< The value of the frame counter. */

    SampleState stateBuffer[AUDIO_BUFFER_LENGTH];
    int stateBufferLength;

    Pulse* pulse1;
    Pulse* pulse2;
    Triangle* triangle;
    Noise* noise;

    float getOutput();
    float mix(uint8_t p1, uint8_t p2, uint8_t t, uint8_t n);
    void stepEnvelope();
    void stepSweep();
    void stepLength();
    void writeControl(uint8_t value);
};

#endif // APU_HPP
