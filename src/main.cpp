#include <Arduino.h>
#include <ThingifyEsp.h>
#include <EasyButton.h>
#include <HLW8012.h>

const int EnergySelPin = 12;
const int EnergyCf1Pin = 4;
const int EnergyCfPin = 5;

const int ButtonPin = 13;
const int RelayPin = 15;

const int RedLedPin = 0;
const int BlueLedPin = 2;

const double CurrentMultiplier = 23744.0;
const double VoltageMultiplier =  252981.0;
const double PowerMultiplier = 2765665.0;

const bool CalibrationMode = false;

EasyButton button(ButtonPin);
ThingifyEsp thing("Blitwolf-SHP6");
Node* switchNode;
Node* currentMp;
Node* voltageMp;
Node* powerMp;

HLW8012 hlw;

void AddCalibrationNodes();
void UpdateCalibrationNodes();

void ICACHE_RAM_ATTR hjl01_cf1_interrupt() 
{
    hlw.cf1_interrupt();
}
void ICACHE_RAM_ATTR hjl01_cf_interrupt() 
{
    hlw.cf_interrupt();
}

bool OnSwitchChanged(void*_, Node *node)
{
  bool value = node->Value.AsBool();
  digitalWrite(RelayPin, value);
  thing["red.led"]->SetValue(NodeValue::Boolean(value));
	return true;
}
bool OnRedChanged(void*_, Node *node)
{
  bool value = node->Value.AsBool();
  digitalWrite(RedLedPin, !value);
	return true;
}
bool OnBlueChanged(void*_, Node *node)
{
  bool value = node->Value.AsBool();
  digitalWrite(BlueLedPin, !value);
	return true;
}

void OnButtonPressed()
{
	auto value = !switchNode->Value.AsBool();
	switchNode->SetValue(NodeValue::Boolean(value));
}
void OnButtonLongPressed()
{
	thing.ResetConfiguration();
}
void setup()
{
	Serial.begin(115200);

  pinMode(RelayPin, OUTPUT);
  pinMode(RedLedPin, OUTPUT);
  digitalWrite(RelayPin, 0);
  digitalWrite(RedLedPin, 1);

  hlw.begin(EnergyCfPin, EnergyCf1Pin, EnergySelPin, LOW, true);

  hlw.setCurrentMultiplier(CurrentMultiplier);
  hlw.setVoltageMultiplier(VoltageMultiplier);
  hlw.setPowerMultiplier(PowerMultiplier);

  attachInterrupt(digitalPinToInterrupt(EnergyCf1Pin), hjl01_cf1_interrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(EnergyCfPin), hjl01_cf_interrupt, FALLING);


  button.begin();
  button.onPressed(OnButtonPressed);
  button.onPressedFor(6000, OnButtonLongPressed);

	thing.AddDiagnostics(1000);
  thing.AddStatusLed(BlueLedPin, true);

  switchNode = thing.AddBoolean("switch")->OnChanged(OnSwitchChanged);
  switchNode->SetValue(NodeValue::Boolean(false));
  thing.AddBoolean("red.led")->OnChanged(OnRedChanged)->SetValue(NodeValue::Boolean(false));
  thing.AddBoolean("blue.led")->OnChanged(OnBlueChanged)->SetValue(NodeValue::Boolean(false));;
  
  thing.AddFloat("current", ThingifyUnit::Ampere);
  thing.AddFloat("power", ThingifyUnit::W);
  thing.AddFloat("volatage", ThingifyUnit::Volt);

  AddCalibrationNodes();
  
	thing.Start();
}

void loop()
{
  button.read();
	thing.Loop();

  static int checkPowerTimer = 0;

  if(millis() - checkPowerTimer < 500)
  {
    return;
  }

  checkPowerTimer = millis();
  thing["current"]->SetValue(NodeValue::Float(hlw.getCurrent()));
  thing["power"]->SetValue(NodeValue::Float(hlw.getActivePower()));
  thing["volatage"]->SetValue(NodeValue::Float(hlw.getVoltage()));

  UpdateCalibrationNodes();  
}

void AddCalibrationNodes()
{
    if(!CalibrationMode)
    {
      return;
    }
    voltageMp = thing.AddFloat("voltage.mp");
    currentMp = thing.AddFloat("current.mp");
    powerMp = thing.AddFloat("power.mp");
}
void UpdateCalibrationNodes()
{
    if(!CalibrationMode)
    {
      return;
    }
    hlw.expectedVoltage(230);
    hlw.expectedActivePower(1800);
    hlw.expectedCurrent(1800.0/230.0);
    voltageMp->SetValue(NodeValue::Float(hlw.getVoltageMultiplier()));
    currentMp->SetValue(NodeValue::Float(hlw.getCurrentMultiplier()));
    powerMp->SetValue(NodeValue::Float(hlw.getPowerMultiplier()));
}
