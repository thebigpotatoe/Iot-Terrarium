void ledInit() {
  // add the leds to fast led and clear them
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(ledString, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  // Debug
  Serial.println("[handleMode] - LED was set up correctly");
}

void tempHumidityInit() {
  dht.begin();

  float _temperature = dht.readTemperature();
  float _humidity = dht.readHumidity();
  // float _temperature = dht.readTemperature(true); // Fahrenheit

  lastTemperatureTime = now();
  lastHumidityTime = now();

  // Check if values are valid
  if (isnan(_humidity) || isnan(_temperature)) {
    return;
  }
  else {
    for (int i = 0; i < NUM_SAMPLES; i++) {
      temperatureArray[i] = _temperature;
      humidityArray[i] = _humidity;
    }
  }
}

void moistureInit() {
  moistureLevel = (float)analogRead(A0)/10.24;
}

void setColour() {
  // Dim the LED's up and down when switching the state
  if (State != previousState) {
    // Dim up when turning on
    if (State) {
      for (int i = 1; i <= 16; i++) {
        //  Set the CRGB Array back to original colours
        fill_solid(ledString, NUM_LEDS, CRGB(colourRed, colourGreen, colourBlue));

        // Scale the brightness 
        nscale8(ledString, NUM_LEDS, (i*16)-1);

        // Show the LED's 
        FastLED.show();

        // Delay for 12 mills
        delay(12);
      }
    }

    // Dim down when turning off
    else {
      for (int i = 15; i >= 0; i--) {
        //  Set the CRGB Array back to original colours
        fill_solid(ledString, NUM_LEDS, CRGB(colourRed, colourGreen, colourBlue));

        // Scale the brightness 
        nscale8(ledString, NUM_LEDS, (i*16));

        // Show the LED's 
        FastLED.show();

        // Delay for 12 mills
        delay(12);
      }
    }

    // Set state to the previous state
    previousState = State;
  }

  // Else always show the LED's when state in is true
  else if (State) {
    if (previousRed != colourRed || previousGreen != colourGreen || previousBlue != colourBlue) {
      // Change the previous colours to the current ones 
      previousRed = colourRed;
      previousGreen = colourGreen;
      previousBlue = colourBlue;

      // Set all LED's to the same colour
      fill_solid(ledString, NUM_LEDS, CRGB(colourRed, colourGreen, colourBlue));
      FastLED.show();
    } 
  }
}

bool readDhtSensor() {
  // Read the sensor
  float _temperature = dht.readTemperature();
  float _humidity = dht.readHumidity();
  // float _temperature = dht.readTemperature(true); // Fahrenheit
  // Serial.println(String(_temperature) + ":" + String(_humidity));

  // Check if values are valid
  if (isnan(_humidity) || isnan(_temperature)) {
    return false;
  }
  else {
    // Store valid vaues in array
    temperatureArray[tempIndex] = _temperature;
    humidityArray[humidityIndex] = _humidity;

    // Increment the array indicies
    tempIndex = (tempIndex + 1) % NUM_SAMPLES;
    humidityIndex = (humidityIndex + 1) % NUM_SAMPLES;

    // Update the last collection time
    lastTemperatureTime = now();
    lastHumidityTime = now();

    // Return true
    return true;
  }
}

bool readMoisture() {
  // Read the ADC and store the value
  moistureLevel = (float)analogRead(A0)/10.24;
  return true;
}

void handleIO() {
  // Set the LED Colours
  setColour();

  // Read all of the sensors
  if (millis() - lastCollectionTime > COLLECTION_PERIOD) {
    if (!readDhtSensor()) {
      Serial.println(F("Failed to read from DHT sensor!"));
    }

    if (!readMoisture()) {
      Serial.println(F("Failed to read from Soil Moisture sensor!"));
    }

    // Send new information to connected clients
    websocketSendArray("Temperature", temperatureArray, tempIndex, lastTemperatureTime);
    websocketSendArray("Humidity", humidityArray, humidityIndex, lastHumidityTime);
    websocketSend(String("{\"Moisture\": " + String(moistureLevel) + " }").c_str());    

    lastCollectionTime = millis();
  }
}
