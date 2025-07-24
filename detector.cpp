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
 * @brief (Live Tracking) Scans for peaks, returning the amplitude and writing the frequency to a pointer.
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
                *out_frequency = static_cast<float>(peak_bin) * max_freq / audio_data_len;
                return peak_value;
            }
        }
    }
    *out_frequency = 0.0;
    return 0; 
}

/**
 * @brief (NEW - Look-back) Finds ALL sharp peaks in a given audio frame.
 * @param peaks_out_buffer A buffer allocated in JS to be filled with found peak frequencies.
 * @param max_peaks The maximum number of peaks the buffer can hold.
 * @return The number of peaks found and written to the buffer.
 */
EXTERN EMSCRIPTEN_KEEPALIVE
int find_all_peaks(Detector* detector, uint8_t* data, int data_len, float sample_rate, int sensitivity, float* peaks_out_buffer, int max_peaks) {
    int peaks_found = 0;
    float max_freq = sample_rate / 2.0;

    // Iterate through all frequency bins, skipping the very low frequencies (noise).
    for (int i = 10; i < data_len - 1; ++i) {
        uint8_t amplitude = data[i];
        
        // Check if this point is a "sharp" peak
        if (amplitude > sensitivity) {
            uint8_t left_neighbor = data[i - 1];
            uint8_t right_neighbor = data[i + 1];
            
            if (amplitude > left_neighbor + 25 && amplitude > right_neighbor + 25) {
                if (peaks_found < max_peaks) {
                    float peak_freq = static_cast<float>(i) * max_freq / data_len;
                    peaks_out_buffer[peaks_found] = peak_freq;
                    peaks_found++;
                } else {
                    // Stop if the output buffer is full
                    break;
                }
            }
        }
    }
    return peaks_found;
}

