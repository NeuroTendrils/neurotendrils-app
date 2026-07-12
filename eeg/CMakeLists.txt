# In-tree EEG inference: bundle I/O, preprocess (FIR + EMS), ONNX Runtime.
# Kept separate from the Qt UI so the pipeline stays readable and reusable.

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/OnnxRuntime.cmake")

include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

add_library(eeg_runtime STATIC
    src/io/bundle_reader.cpp
)

target_include_directories(eeg_runtime
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(eeg_runtime
    PUBLIC
        nlohmann_json::nlohmann_json
)

add_library(eeg_inference STATIC
    src/inference/onnx_runner.cpp
    src/preprocess/window_buffer.cpp
    src/preprocess/preprocessor.cpp
)

target_include_directories(eeg_inference
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(eeg_inference
    PUBLIC
        eeg_runtime
)

link_onnxruntime(eeg_inference)
