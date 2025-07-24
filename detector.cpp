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

EXTERN EMSCRIPTEN_KEEPALIVE
float detector_find_peak(Detector* detector, uint8_t* data, int data_len, float sample_rate, float target_freq_hz, int sensitivity) {
    float max_freq = sample_rate / 2.0;
    int target_bin = static_cast<int>(round(target_freq_hz * data_len / max_freq));

    int window_size = 5;
    int start_bin = fmax(1, target_bin - window_size);
    int end_bin = fmin(data_len - 1, target_bin + window_size);

    uint8_t peak_value = 0;
    int peak_bin = -1;

    for (int i = start_bin; i <= end_bin; ++i) {
        if (data[i] > peak_value) {
            peak_value = data[i];
            peak_bin = i;
        }
    }

    if (peak_bin != -1 && peak_value > sensitivity) {
        uint8_t left_neighbor = (peak_bin > 0) ? data[peak_bin - 1] : 0;
        uint8_t right_neighbor = (peak_bin < data_len - 1) ? data[peak_bin + 1] : 0;
        
        if (peak_value > left_neighbor + 20 && peak_value > right_neighbor + 20) {
            return static_cast<float>(peak_bin) * max_freq / data_len;
        }
    }

    return 0.0;
}

/**
 * @brief (Updated Function) Scans the ENTIRE buffer to find the single strongest, sharpest peak.
 * It now accepts a sensitivity parameter to tune it for noisy environments.
 * @param out_peak_freq A pointer to a float where the result frequency will be stored.
 * @param sensitivity The minimum amplitude (0-255) a peak must have to be considered.
 * @return The amplitude (0-255) of the strongest peak found, or 0 if none.
 */
EXTERN EMSCRIPTEN_KEEPALIVE
uint8_t find_strongest_peak(Detector* detector, uint8_t* data, int data_len, float sample_rate, float* out_peak_freq, int sensitivity) {
    uint8_t max_amplitude = 0;
    int best_peak_bin = -1;

    // Iterate through all frequency bins, skipping the very low frequencies (noise).
    for (int i = 10; i < data_len; ++i) {
        uint8_t amplitude = data[i];

        // Check if this point is a potential peak worth considering.
        // It must be stronger than the user-defined sensitivity and the current max.
        if (amplitude > sensitivity && amplitude > max_amplitude) {
            // Check if it's a "sharp" peak (stronger than its neighbors).
            uint8_t left_neighbor = (i > 0) ? data[i - 1] : 0;
            uint8_t right_neighbor = (i < data_len - 1) ? data[i + 1] : 0;
            
            // This heuristic helps reject broadband noise in favor of tonal sounds.
            if (amplitude > left_neighbor + 25 && amplitude > right_neighbor + 25) {
                max_amplitude = amplitude;
                best_peak_bin = i;
            }
        }
    }

    if (best_peak_bin != -1) {
        float max_freq = sample_rate / 2.0;
        // Write the frequency of the best peak to the output pointer.
        *out_peak_freq = static_cast<float>(best_peak_bin) * max_freq / data_len;
        return max_amplitude;
    }

    // No peak found, write 0.0 to the frequency and return 0 amplitude.
    *out_peak_freq = 0.0;
    return 0;
}

