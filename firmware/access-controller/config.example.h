#pragma once

#include <stdint.h>

// Copia este archivo como config.h y sustituye los valores de ejemplo.
// config.h está excluido de Git para evitar publicar credenciales.

constexpr char WIFI_SSID[] = "YOUR_WIFI_SSID";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";

constexpr char MQTT_HOST[] = "broker.example.com";
constexpr uint16_t MQTT_PORT = 1883;
constexpr char MQTT_USERNAME[] = "YOUR_MQTT_USERNAME";
constexpr char MQTT_PASSWORD[] = "YOUR_MQTT_PASSWORD";
constexpr char MQTT_TOPIC_PREFIX[] = "parking";
