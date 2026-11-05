#include <Arduino.h>

// TensorFlow Lite libraries
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "sine_model.h"  

#define LED_PIN 2
#define DEBUG 0

const float pi     = 3.1415926535897932384626433832795f;
const float freq   = 0.5f;                  // Hz
const float period = (1.0f / freq) * 1000;  // milliseconds

namespace {
  tflite::ErrorReporter*   error_reporter = nullptr;
  const tflite::Model*     model          = nullptr;
  tflite::MicroInterpreter* interpreter   = nullptr;
  TfLiteTensor* model_input               = nullptr;
  TfLiteTensor* model_output              = nullptr;

  constexpr int kTensorArenaSize = 5 * 1024;
  uint8_t tensor_arena[kTensorArenaSize];
} // namespace

void setup() {
  Serial.begin(115200);
  #if DEBUG
    while (!Serial);
  #endif

  pinMode(LED_PIN, OUTPUT);

  // Error reporter
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  // Load model
  model = tflite::GetModel(sine_model_tflite);

  // Register ops
  static tflite::MicroMutableOpResolver<1> micro_op_resolver;
  micro_op_resolver.AddFullyConnected();

  // Build interpreter (must be static so it outlives setup())
  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  // Allocate tensors
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    error_reporter->Report("AllocateTensors() failed");
    while (1);
  }

  // Get pointers to input/output tensors
  model_input  = interpreter->input(0);
  model_output = interpreter->output(0);

  #if DEBUG
    Serial.print("Number of dimensions: ");
    Serial.println(model_input->dims->size);
    Serial.print("Dim 1 size: ");
    Serial.println(model_input->dims->data[0]);
    Serial.print("Dim 2 size: ");
    Serial.println(model_input->dims->data[1]);
    Serial.print("Input type: ");
    Serial.println(model_input->type);
  #endif
}

void loop() {
  #if DEBUG
    unsigned long start_timestamp = millis();
  #endif

  // Get current time position within one period
  unsigned long timestamp = millis() % (unsigned long)period;

  // Map to x in [0, 2π]
  float x_val = ((float)timestamp * 2.0f * pi) / period;

  // Feed input
  model_input->data.f[0] = x_val;

  // Run inference
  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    error_reporter->Report("Invoke failed on input: %f\n", x_val);
  }

  // Read output
  float y_val = model_output->data.f[0];

  // Drive LED brightness (sine output is -1 to 1, clamp to 0-255)
  int brightness = (int)(127.5f + 127.5f * y_val);  // maps [-1,1] -> [0,255]
  brightness = constrain(brightness, 0, 255);
  analogWrite(LED_PIN, brightness);

  Serial.print("x: ");
  Serial.print(x_val);

  Serial.print(", ");

  Serial.print("y: ");
  Serial.println(y_val);

  delay(100);

  #if DEBUG
    Serial.print("Time for inference (us): ");
    Serial.println(millis() - start_timestamp);
  #endif
}