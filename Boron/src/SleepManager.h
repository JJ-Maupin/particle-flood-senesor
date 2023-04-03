#ifndef _SLEEPMANAGER_H_
#define _SLEEPMANAGER_H_

class SleepManager{
    private:
        long sleepPeriod = 15 * 60 * 1000; //In sec, default 15 min.
        long delayt = 2 * 60 * 1000; //In ms, defualt 2min.
        bool allowSlp = true,checkBatLvl = true;
        
        FuelGauge fuelGauge;
        SystemSleepConfiguration config; 

        long long tNowMiliseconds(); //Convert currtime in sec to miliseconds.
        long tUntilNextInterval(); //Will return time to next intercal in ms.
        void deepSlp(); //Will configure deep sleep on current state and sleep.

    public:
        void checkBatteryLevel(bool);//Battery protection function.
        void setSleepPeriod(float);//Set time in miliseconds.
        void resetSleepPeriod();//Reset to default sleep value.
        void firstSleep();//Function for delay at bootup.
        void sleepFunction();//Function to manage sleep.
        void allowSleep(bool);//Set boron to be allowed to sleep true=allowed.
        bool isSleepAllowed();//Get allSlp.
        void checkBatteryLevel();//Check if battery is below threshold .
};

void SleepManager::setSleepPeriod(float timeInMilisecond) {
    sleepPeriod = timeInMilisecond;
}

void SleepManager::resetSleepPeriod() {
    sleepPeriod = 15 * 60 * 1000;
}

//Delay for first boot -> currently unused.
void SleepManager::firstSleep() {
    if(allowSlp){
        delay(tUntilNextInterval() / 1000); //Delay in seconds.
    }
}

void SleepManager::sleepFunction() {
    Particle.process();
    if (allowSlp) {
        Serial.print("Delaying: ");
        Serial.println(delayt);
        //delay(delayt);
        for(int i=0; i < delayt /1000; i++){
            delay(1000);
        }
        deepSlp();
    }
}

void SleepManager::allowSleep(bool allow) {
    allowSlp = allow;
}

bool SleepManager::isSleepAllowed() {
    return allowSlp;
}

//DOES NOT CHECK BATTRY INTEGRITY ONLY PREVENTS SLEEP IF LOW POWER
void SleepManager::checkBatteryLevel() {
    if (checkBatLvl && fuelGauge.getSoC() < 20) {
        deepSlp();
    }
}

void SleepManager::checkBatteryLevel(bool var) {
    checkBatLvl = var;
}

long SleepManager::tUntilNextInterval() {
    Particle.process();
    long amount = sleepPeriod - (tNowMiliseconds() % sleepPeriod);
    while(!Particle.publish("SleepEvent", "{Sleep time: " + String((int)amount) + "}", PUBLIC, WITH_ACK )){}
    amount = sleepPeriod - (tNowMiliseconds() % sleepPeriod);
    Serial.print("\tnow: ");
    Serial.print(String((long)Time.now()));
    Serial.print("\tCurrPeriod: ");
    Serial.print(String(sleepPeriod));
    Serial.print("\tCurrtime: ");
    Serial.print(tNowMiliseconds());
    Serial.print("\t ETA: " );
    Serial.println(String(amount/1000/60)); 
    return amount;
}

void SleepManager::deepSlp() {
    if (Time.now() < 10000000) { //Should only trigger when battery is low and a time fix has not been astablished
        config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(sleepPeriod);
        Serial.println("Commit To Wrong Time Sleep i.e. delay interval");
        System.sleep(config);
    } else {
        config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(tUntilNextInterval());
        Serial.println("Commit To Sleep");
        System.sleep(config);
    }
}

long long SleepManager::tNowMiliseconds() {
    return (((long long)Time.now()) * 1000);
}
#endif
