#pragma once
#include "SyncTest.h"
#include "time.h"
#include "math.h"
using namespace std;

int SyncTest::checkAngle(double desired, bool *bitFailed)
{
    int errNbr = 0;
    bool BITn;

    dut->da->setAngle(desired);
    double lastError = 360.0;
    int i;
    double measured, error;
    *bitFailed = false;

    // wait until we reach the target
    for (i = 0; i < 1000; i++) {
        measured = dut->sync->getPos();
        error = abs(getAngularError(desired, measured));
        if (!dut->sync->isBITnValid()) *bitFailed = true;
        if (verbose) printf("time %d: angle: %f, error = %f\n", i, measured, error);
        if (error >= lastError && error < 1) break;
        lastError = error;
        Sleep(1);
    }
    BITn = dut->sync->isBITnValid();
    int timeOut = 5;
    while (!(BITn = dut->sync->isBITnValid()) && (timeOut-- > 0)) {
        usleep(700);
    }
    if (!BITn)
    {
        printf("Error : BITn invalid!\n");
        errNbr++;
    }

    if (lastError > 1) {
        printf("Error: Could not find the position: desired %f deg., measured %f deg. Error %f deg.\n", desired, measured, error);
        errNbr++;
    }
    else {
        printf("Success: Checking angle %f, Measured %f deg., error %f deg. settling time: %d ms\n", desired, measured, error, i);
    }
    if (verbose || (errNbr > 0)) {
        printf("Gains 0x%04x, 0x%04x, 0x%04x\n", dut->da->getSimGain(0),
            dut->da->getSimGain(1), dut->da->getSimGain(2)); // 0x2121
        uint32_t angVel = dut->da->getAngularVelocityRaw();
        printf("Angular Velocity %d (0x%x) = %f Rad/clk\n", angVel, angVel, dut->da->getAngularVelocity());
        printf("Frequency %f Hz\n", dut->da->getFrequency());
        printf("Converter Busy Time %x\n", dut->sync->getBusyTime());
        printf("Converter Cycle Time %x\n", dut->sync->getCycleTime());
        printf("Encoder Counter %x\n", dut->sync->getEncoderCounter());
    }
    return errNbr;
}

int SyncTest::staticLoopbackTest()
{
    int nbrErrors = 0;
    bool bitChanged;

    dut->da->setFrequency(4000);
//    dut->da->setFrequency(64190);

    for (int a = 0; a <= 360; a += 45) {
        nbrErrors += checkAngle(a % 360, &bitChanged);
    }
    return nbrErrors;
}

/** @brief Make sure that an angle in degree is between -180 and +180
* @param angle Angle to normalize in degrees.
* @retval the equivalent angle between -180 and 180.
 */
double SyncTest::normalizeAngle(double angle)
{
    while (angle > 180.0) {
        angle -= 360.0;
    }
    while (angle < -180.0) {
        angle += 360.0;
    }
    return angle;
}


/** @brief return error in degree
 * @retval a signed error
 */
double SyncTest::getAngularError(double desired, double actual)
{
    return normalizeAngle(normalizeAngle(desired) - normalizeAngle(actual));
}

int SyncTest::dynamicLoopbackTest()
{
    int nbrErrors = 0;

#define SPEED	80.0
    double res_angle, res_error;
    uint16_t last_enc;
    bool BITn;
    struct timespec tstart = { 0,0 }, tend = { 0,0 };
    last_enc = dut->sync->getEncoderCounter();
    double maxErrorPos = 0;
    double maxErrorNeg = 0;

    //    dut->da->setFrequency(64190);
    double simulatorFrequency = 4000;
    dut->da->setFrequency(simulatorFrequency);
    printf("Simulator frequency requested: %f, angular velocity 0x%08x\n",
        simulatorFrequency, dut->da->getAngularVelocityRaw());
    dut->da->setAngle(45);
    Sleep(100);

    clock_gettime(0, &tstart);

    printf("\nTracking test: ");
    for (double angle = 45; angle < 405 && nbrErrors <10; angle += 1/ SPEED)
    {
        bool shouldDisplay = false;
        double desiredAngle = angle;
        while (desiredAngle >= 360) desiredAngle -= 360;

        dut->da->setAngle(desiredAngle);
        usleep(700);

        // wait for BIT to be high
        BITn = dut->sync->isBITnValid();
        int timeOut = 50;
        while (!(BITn = dut->sync->isBITnValid())&& (timeOut-- > 0)){
            usleep(300);
        }

        res_angle = dut->sync->getPos();
        res_error = getAngularError(res_angle, desiredAngle);
        if (res_error > 180) res_error -= 360;
        while ((abs(res_error) > 1.0) && (timeOut-- > 0)) {
            res_angle = dut->sync->getPos();
            res_error = res_angle - desiredAngle;
            if (res_error > 180) res_error -= 360;
            usleep(300);
        }

        if (timeOut <= 0)
        {
            shouldDisplay = true;
            printf("Time out!\n");
            nbrErrors++;
        }
        if (res_error > maxErrorPos) {
            maxErrorPos = res_error;
        }
        if (res_error < maxErrorNeg) {
            maxErrorNeg = res_error;
        }

        if (res_error > 2.0 || res_error < -2.0) {
            nbrErrors++;
            shouldDisplay = true;
        }
        if (shouldDisplay) {
            printf("Desired Angle %f -- Resolver Angle %f\n", desiredAngle, res_angle);
            printf("BIT %s -- Error(degrees) = %f, delay = %d\n", 
                BITn ? "OK":"Failed", res_error, (20 - timeOut)*300);
        }
    }
    clock_gettime(0, &tend);

    if (nbrErrors > 0)
    {
        printf("Error: Tracking test failed!\n");
    }
    else {
        printf("Success!\n");
    }
    printf("One rotation took about %.5f seconds, maximum error=%f to %f\n",
        ((double)tend.tv_sec + 1.0e-9 * tend.tv_nsec) -
        ((double)tstart.tv_sec + 1.0e-9 * tstart.tv_nsec),
        maxErrorNeg, maxErrorPos);

    return nbrErrors;
}

int SyncTest::checkAngleToRadConv(double test)
{
    int nbrErrors = 0;
    int angle = dut->da->radianToAngle(test);
    double rad = dut->da->angleToRadian(angle);
    if (rad != test) {
        printf("Failed: angle conversion for %f radians - angle=0x%05x - rad=%f\n",
            test, angle, rad);
        nbrErrors++;
    }
    return nbrErrors;
}

int SyncTest::checkAngleToGain(double desiredAngle, int16_t gain1, int16_t gain2, int16_t gain3)
{
    int nbrErrors = 0;

    // set the angle 
    dut->da->setAngle(desiredAngle);

    // compare the gains
    int16_t gain;
    gain = dut->da->getSimGain(0);
    if (gain != gain1) {
        printf("Failed: angle conversion for %f degrees - gain=%d - expected=%d\n",
            desiredAngle, gain, gain1);
        nbrErrors++;
    }
    gain = dut->da->getSimGain(1);
    if (gain != gain2) {
        printf("Failed: angle conversion for %f degrees - gain=%d - expected=%d\n",
            desiredAngle, gain, gain2);
        nbrErrors++;
    }
    gain = dut->da->getSimGain(2);
    if (gain != gain3) {
        printf("Failed: angle conversion for %f degrees - gain=%d - expected=%d\n",
            desiredAngle, gain, gain3);
        nbrErrors++;
    }
    return nbrErrors;
}

int SyncTest::miscSoftwareVerifications()
{
    int nbrErrors = 0;

    printf("Angle conversion test: ");
    nbrErrors += checkAngleToRadConv(-3);
    nbrErrors += checkAngleToRadConv(-2);
    nbrErrors += checkAngleToRadConv(-1.5);
    nbrErrors += checkAngleToRadConv(-1);
    nbrErrors += checkAngleToRadConv(1);
    nbrErrors += checkAngleToRadConv(1.5);
    nbrErrors += checkAngleToRadConv(2);
    nbrErrors += checkAngleToRadConv(3);
    nbrErrors += checkAngleToGain(0, -0x3fff, 0x7fff, -0x3fff);
    nbrErrors += checkAngleToGain(90, 0x9127, 0x0000, 0x6ed9);
    nbrErrors += checkAngleToGain(180, 0x3fff, -0x7fff, 0x3fff);
    nbrErrors += checkAngleToGain(270, 0x6ed9, 0x0000, 0x9127);



    if (nbrErrors==0)
        printf("Passed!\n");
    else
        printf("Failed!");

    return nbrErrors;
}

/** @brief Initialize the value table
 * 
 * @param angle in degree. This is used to calculate the respective gains.
 */
void SyncTest::spiDaInitSineTables(double angle)
{
    double s1_angle;
    double gain1, gain2, gain3;


    s1_angle = -(angle + 30);

    gain1 = sin(degToRad(s1_angle));
    gain2 = sin(degToRad(s1_angle + 120));
    gain3 = sin(degToRad(s1_angle + 240));

    double spiDaAngularVelocity = (2 * M_PI) / (double)spiDaNbrOfValues;

    for (int i = 0; i < spiDaNbrOfValues; i++) {
        double sinus = sin(spiDaAngularVelocity * i);

        synchRef[i] = (int16_t)(sinus * 32767);
        synchS1[i] = (int16_t)(sinus * gain1 * 32767);
        synchS2[i] = (int16_t)(sinus * gain2 * 32767);
        synchS3[i] = (int16_t)(sinus * gain3 * 32767);
//        printf("%d\t%d\t%d\t%d\n", synchRef[i]+65000, synchS1[i] + 65000, synchS2[i] + 65000, synchS3[i] + 65000);
    }

}

inline long timediff(clock_t t1, clock_t t2) {
    long elapsed;
    elapsed = (long)(((double)t2 - t1) / CLOCKS_PER_SEC * 1000);
    return elapsed;
}

int SyncTest::tryAngle(double desired)
{
    double measured = dut->sync->getPos();
    double error = 0, prevMeasured = 0;
    clock_t t1, t2;
    long prevDiff = 0;
    int verbose = 0;
    double lastError = 360.0;
    long diff = 0, lastDiff = -1;

    printf("Trying %f\n", desired);
    spiDaInitSineTables(desired);      // initialize the wave form tables
    bool waveform = true;

    // we will send it for 200 milliseconds. We should be able to synchronize by then.
    dut->controlRegister->setData(ControlRegister::CTRL_DaMode_mask);
    dut->spi_da->reset();
    t1 = clock();
    for (int i = 0; i < 1000000; i++) {
        t2 = clock();
        diff = timediff(t1, t2);
        if (diff > 3000) {
            break;
        }
        usleep(300);

#define WAVEFORM
#ifdef WAVEFORM
        int index = diff % spiDaNbrOfValues;

        dut->spi_da->setCode(synchRef[index], Ltc2664_SPI::CHANNEL_0, Ltc2664_SPI::CHANNEL_NONE, false);
        dut->spi_da->setCode(synchS1[index], Ltc2664_SPI::CHANNEL_1, Ltc2664_SPI::CHANNEL_NONE, false);
        dut->spi_da->setCode(synchS2[index], Ltc2664_SPI::CHANNEL_2, Ltc2664_SPI::CHANNEL_NONE, false);
        dut->spi_da->setCode(synchS3[index], Ltc2664_SPI::CHANNEL_3, Ltc2664_SPI::CHANNEL_ALL, false);
#else            if (i == 0) {
        dut->spi_da->setCode(32767, Ltc2664_SPI::CHANNEL_0, Ltc2664_SPI::CHANNEL_NONE, false);
        dut->spi_da->setCode((int16_t)(sin(degToRad(desired)) * 32767),
            Ltc2664_SPI::CHANNEL_1, Ltc2664_SPI::CHANNEL_NONE, false);
        dut->spi_da->setCode((int16_t)(sin(degToRad(desired + 120)) * 32767),
            Ltc2664_SPI::CHANNEL_2, Ltc2664_SPI::CHANNEL_NONE, false);
        dut->spi_da->setCode((int16_t)(sin(degToRad(desired + 240)) * 32767),
            Ltc2664_SPI::CHANNEL_3, Ltc2664_SPI::CHANNEL_ALL, false);
#endif

        measured = dut->sync->getPos();
        error = abs(getAngularError(desired, measured));
        if (verbose) {
            if (abs(getAngularError(measured, prevMeasured)) > 1 && diff-prevDiff >20) {
                printf("time %d: angle: %f, error = %f, bit = %s\n",
                    diff, measured, error, dut->sync->isBITnValid() ? "OK" : "Fail");
                prevMeasured = measured;
                prevDiff = diff;
            }
        }
        if (error >= lastError && error < 1) break;
        lastError = error;

    }

    error = abs(getAngularError(desired, measured));
    printf("Exiting: time %d: angle: %f, error = %f, bit = %s\n",
        diff, measured, error, dut->sync->isBITnValid() ? "OK" : "Fail");
    return 0;
}

int SyncTest::testSpiDa()
{
    int nbrErrors = 0;

    cout << "SPI Read-back Test: ";
    dut->sync->reset();

    uint32_t data = dut->controlRegister->getData();
    //dut->enableSpiDa(true);
    //dut->controlRegister->setData(data | ControlRegister::gpOut_DaMode_mask);
 //   dut->controlRegister->setData(ControlRegister::gpOut_DaMode_mask | ControlRegister::gpOut_Ldac_mask);
    dut->controlRegister->setData(ControlRegister::CTRL_DaMode_mask);
    dut->spi_da->reset();

    dut->spi_da->setCode(0x1234, Ltc2664_SPI::CHANNEL_2, Ltc2664_SPI::CHANNEL_2, true);
/*    //Read back should be: 0x0321234
    uint32_t* result = dut->spi_da->buff_in;
    if (result[1] == 0x32 && result[2] == 0x12 && result[3] == 0x34)
    {
        cout << "Passed!" << endl;
    }
    else {
        cout << "Failed! should read 0x00321234, read 0x00" << hex << (int)result[1] << (int)result[2] << (int)result[3] << endl;
        return 1;   // we failed, no reason to keep testing.
    }
    */
    // check if we can send waveforms that the synchro will recognize.
    tryAngle(45);      // initialize the wave form tables
    tryAngle(50);      // initialize the wave form tables
    tryAngle(55);      // initialize the wave form tables
    tryAngle(60);      // initialize the wave form tables
    tryAngle(65);      // initialize the wave form tables

    dut->sync->reset();
    dut->spi_da->reset();

    /**/
//    dut->enableSpiDa(false);
    dut->controlRegister->setData(0);

     return nbrErrors;
}

static bool irqReceived;
void bitIntReceived(void* brd, UINT32 data)
{
    // acknowledge the interrupt
    irqReceived = true;

    Sleep(1);
}

int SyncTest::irqTest()
{
    // check that there is not interrupt already being requested
    uint16_t irqStatus = dut->cra->getIrqStatus();
    uint16_t irqEnable = dut->cra->getIrqEnableMask();
    if (irqStatus != 0 || irqEnable != 0) {
        cout << "Warning: while entering, interrupter not initialized: pcieIrqStatus = 0x"
            << hex << irqStatus << ", pcieIrqEnable = 0x" << irqEnable << endl;
    }

    int nbrErrors = 0;
    irqReceived = false;
    cout << "BITn Interrupt Test: ";

    // allow the interrupts
    PCIeMini_status st = dut->hookInterruptServiceRoutine((MINIPCIE_INT_HANDLER)bitIntReceived);
    if (st != WD_STATUS_SUCCESS)
        cout << "hookInterruptServiceRoutine failed, error #" << st << endl;

    dut->statusRegister->clearEdgeCapture(0xffffffff);
    dut->statusRegister->enableBitValidIrq();       // Enable the PIO interrupt
    st = dut->enableInterrupts(dut->irq_mask_statusReg);             // Enable the PCIe interrupt and Windows

    bool bitHasBeenInvalid = false;
    checkAngle(0, &bitHasBeenInvalid);
    checkAngle(180, &bitHasBeenInvalid);
    checkAngle(0, &bitHasBeenInvalid);
    if (irqReceived) {
        cout << "Success, interrupt received" << endl;
    }
    else {
        cout << "BITn has " << (bitHasBeenInvalid ? "" : "not ")
            << "been asserted. Interrupt was not received." << endl;
        nbrErrors++;
    }
    dut->statusRegister->disableBitValidIrq();
    st = dut->disableInterrupts();
    return nbrErrors;
}

int SyncTest::mainTest(int brdNbr)
{
	int loopCntr = 0;
	int totalError = 0;

	PCIeMini_status st = dut->open(brdNbr);

	cout << "Opening the PCIeMini_Sync: " << getAlphiErrorMsg(st) << endl;
	if (st != ERRCODE_NO_ERROR) {
		cout << "Exiting: Press <Enter> to exit." << endl;
        getc(stdin);
        exit(0);
	}

	cout << "FPGA ID: 0x" << hex << dut->getFpgaID() << endl;
	cout << "FPGA Time Stamp: 0x" << dut->getFpgaTimeStamp() << endl;
    time_t ts = dut->getFpgaTimeStamp();
    cout << "FPGA Time Stamp: 0x" << ts << endl;
    char buff[20];
    strftime(buff, 20, "%m/%d/%Y %H:%M:%S", localtime(&ts));
    printf("%s\n", buff);

	cout << endl << "---------------------------------" << endl;
	cout << "PCIeMini-Sync Confidence Test" << endl;
	cout << "---------------------------------" << endl;
	cout << endl << "use ? as a parameter for list of options." << endl;
	cout << "Options selected" << endl;
	cout << "================" << endl;
	cout << "Using board number " << brdNbr << endl;
	if (verbose) cout << "verbose mode " << endl;

	while (loopCntr <= 100) {
		cout << std::dec << endl << endl << "PCIeMini-Synchro Test Loop # " << loopCntr << " errors = " << totalError << endl;
        totalError += staticLoopbackTest();
        totalError += irqTest();
        totalError += testSpiDa();
        totalError += dynamicLoopbackTest();
        totalError += miscSoftwareVerifications();
        loopCntr++;
		cout << std::dec << endl << "Finished Loop #" << loopCntr << " errors = " << totalError << endl;
		Sleep(0000);
	}
	dut->disableInterrupts();
	dut->close();

	return 0;

}