#include <emscripten.h>
#include <cstdint>
#include <cmath>

#define EXTERN extern "C"

struct Detector {
    int last_peak_bin;
};

EXTERN EMSCRIPTEN_KEEPALIVE
Detector* detector_new() {
    return new Detector{0};
}

EXTERN EMSCRIPTEN_KEEPALIVE
void detector_free(Detector* detector) {
    delete detector;
}

/**
 * @brief (Updated) Scans for peaks, returning the amplitude and writing the frequency to a pointer.
 * @param out_frequency A pointer to a float where the result frequency will be stored.
 * @return The amplitude (0-255) of the FIRST detected peak, or 0 if no peak is found.
 */
EXTERN EMSCRIPTEN_KEEPALIVE
uint8_t detector_find_multiple_peaks(Detector* detector, uint8_t* audio_data, int audio_data_len, float sample_rate, float* target_freqs_hz, int num_target_freqs, int sensitivity, float* out_frequency) {
    float max_freq = sample_rate / 2.0;

    for (int i = 0; i < num_target_freqs; ++i) {
        float target_freq = target_freqs_hz[i];
        int target_bin = static_cast<int>(round(target_freq * audio_data_len / max_freq));

        int window_size = 5;
        int start_bin = fmax(1, target_bin - window_size);
        int end_bin = fmin(audio_data_len - 1, target_bin + window_size);

        uint8_t peak_value = 0;
        int peak_bin = -1;

        for (int j = start_bin; j <= end_bin; ++j) {
            if (audio_data[j] > peak_value) {
                peak_value = audio_data[j];
                peak_bin = j;
            }
        }

        if (peak_bin != -1 && peak_value > sensitivity) {
            uint8_t left_neighbor = (peak_bin > 0) ? audio_data[peak_bin - 1] : 0;
            uint8_t right_neighbor = (peak_bin < audio_data_len - 1) ? audio_data[peak_bin + 1] : 0;
            
            if (peak_value > left_neighbor + 20 && peak_value > right_neighbor + 20) {
                // A peak was found. Write the frequency to the output pointer.
                *out_frequency = static_cast<float>(peak_bin) * max_freq / audio_data_len;
                // Return the amplitude of the peak.
                return peak_value;
            }
        }
    }

    // No peak found.
    *out_frequency = 0.0;
    return 0; 
}


/**
 * @brief (Look-back Function) Scans the ENTIRE buffer to find the single strongest, sharpest peak.
 * @param out_peak_freq A pointer to a float where the result frequency will be stored.
 * @return The amplitude (0-255) of the strongest peak found, or 0 if none.
 */
EXTERN EMSCRIPTEN_KEEPALIVE
uint8_t find_strongest_peak(Detector* detector, uint8_t* data, int data_len, float sample_rate, float* out_peak_freq) {
    uint8_t max_amplitude = 0;
    int best_peak_bin = -1;

    for (int i = 10; i < data_len; ++i) {
        uint8_t amplitude = data[i];
        if (amplitude > 170 && amplitude > max_amplitude) {
            uint8_t left_neighbor = (i > 0) ? data[i - 1] : 0;
            uint8_t right_neighbor = (i < data_len - 1) ? data[i + 1] : 0;
            if (amplitude > left_neighbor + 25 && amplitude > right_neighbor + 25) {
                max_amplitude = amplitude;
                best_peak_bin = i;
            }
        }
    }

    if (best_peak_bin != -1) {
        float max_freq = sample_rate / 2.0;
        *out_peak_freq = static_cast<float>(best_peak_bin) * max_freq / data_len;
        return max_amplitude;
    }

    *out_peak_freq = 0.0;
    return 0;
}

