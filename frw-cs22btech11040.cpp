#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <random>
#include <ctime>
#include <semaphore.h>
#include <iomanip>
#include <sstream>
#include <vector>
#include <numeric>
#include <algorithm>

using namespace std;

// Global variables
int readcount = 0, writecount = 0;
sem_t resource, rmutex, serviceQueue, mut; // Declare mut semaphore globally
ofstream fairRWLogFile;
vector<long long> worstCaseReaderFairRW, worstCaseWriterFairRW;

// Function prototypes
void reader(int id, int kr, int muCS, int muRem, vector<long long>& entryTimes, ofstream& logFile);
void writer(int id, int kw, int muCS, int muRem, vector<long long>& entryTimes, ofstream& logFile);
long long getSysTime();
string formatTime(long long timestamp);
void logEvent(ofstream& logFile, const string& event);
void writeAverageTimeToFile(ofstream& avgTimeFile, const vector<long long>& entryTimes, const string& algorithm);
void writeWorstCaseTimesToFile(ofstream& avgTimeFile, const vector<long long>& worstCaseTimes, const string& algorithm);

int main() {
    // Open input parameters file
    ifstream inputFile("inp-params.txt");
    // Read parameters from file
    int nw, nr, kw, kr;
    int muCS, muRem;
    inputFile >> nw >> nr >> kw >> kr >> muCS >> muRem;
    inputFile.close();

    // Initialize mut semaphore
    sem_init(&mut, 0, 1);

    // Open output log file
    fairRWLogFile.open("FairRW-log.txt");
    ofstream avgTimeFile("AverageTimeFairRW.txt");

    // Initialize semaphores
    sem_init(&resource, 0, 1);
    sem_init(&rmutex, 0, 1);
    sem_init(&serviceQueue, 0, 1);

    // Seed for random number generation
    srand(time(nullptr));

    // Vector to store entry times for calculating average time
    vector<long long> entryTimesFairRW, entryTimesWriterFairRW;

    // Create writer threads for Fair RW algorithm
    thread writerThreadsFairRW[nw];
    for (int i = 0; i < nw; ++i) {
        writerThreadsFairRW[i] = thread(writer, i + 1, kw, muCS, muRem, ref(entryTimesWriterFairRW), ref(fairRWLogFile));
    }

    // Create reader threads for Fair RW algorithm
    thread readerThreadsFairRW[nr];
    for (int i = 0; i < nr; ++i) {
        readerThreadsFairRW[i] = thread(reader, i + 1, kr, muCS, muRem, ref(entryTimesFairRW), ref(fairRWLogFile));
    }

    // Join writer threads for Fair RW algorithm
    for (int i = 0; i < nw; ++i) {
        writerThreadsFairRW[i].join();
    }

    // Join reader threads for Fair RW algorithm
    for (int i = 0; i < nr; ++i) {
        readerThreadsFairRW[i].join();
    }

    // Calculate and write average time for Fair RW algorithm reader and writer threads
    writeAverageTimeToFile(avgTimeFile, entryTimesFairRW, "Fair RW Reader");
    writeAverageTimeToFile(avgTimeFile, entryTimesWriterFairRW, "Fair RW Writer");

    // Write worst-case times to file
    writeWorstCaseTimesToFile(avgTimeFile, worstCaseReaderFairRW, "Fair RW Reader");
    writeWorstCaseTimesToFile(avgTimeFile, worstCaseWriterFairRW, "Fair RW Writer");

    // Close output log file
    fairRWLogFile.close();
    avgTimeFile.close();

    // Destroy semaphores
    sem_destroy(&resource);
    sem_destroy(&rmutex);
    sem_destroy(&serviceQueue);
    sem_destroy(&mut);

    return 0;
}

void reader(int id, int kr, int muCS, int muRem, vector<long long>& entryTimes, ofstream& logFile) {
    long long worstCase = numeric_limits<long long>::min(); // Initialize to minimum possible value
    for (int i = 0; i < kr; ++i) {
        auto reqTime = getSysTime();
        sem_wait(&mut);
        logEvent(logFile, to_string(i + 1) + "st CS request by Reader Thread " + to_string(id));
        sem_post(&mut);
        sem_wait(&serviceQueue);  // Wait in line to be serviced
        sem_wait(&rmutex);        // Request exclusive access to readcount
        readcount++;              // Update count of active readers
        if (readcount == 1) {
            sem_wait(&resource);  // Request resource access for readers (writers blocked)
        }
        sem_post(&serviceQueue);  // Let next in line be serviced
        sem_post(&rmutex);        // Release access to readcount

        auto enterTime = getSysTime();
        sem_wait(&mut);
        logEvent(logFile, to_string(i + 1) + "st CS Entry by Reader Thread " + to_string(id));
        sem_post(&mut);

        // Calculate entry time and store it
        entryTimes.push_back(enterTime - reqTime);

        // Calculate worst-case waiting time
        long long waitingTime = enterTime - reqTime;
        if (waitingTime > worstCase) {
            worstCase = waitingTime;
        }

        // Simulate reading in CS
        this_thread::sleep_for(chrono::milliseconds(rand() % muCS));

        sem_wait(&rmutex);  // Request exclusive access to readcount
        readcount--;        // Update count of active readers
        if (readcount == 0) {
            sem_post(&resource);  // Release resource access for all
        }
        sem_post(&rmutex);  // Release access to readcount

        auto exitTime = getSysTime();
        sem_wait(&mut);
        logEvent(logFile, to_string(i + 1) + "st CS Exit by Reader Thread " + to_string(id));
        sem_post(&mut);

        // Simulate remaining time outside CS
        this_thread::sleep_for(chrono::milliseconds(rand() % muRem));
    }
    sem_wait(&mut); // Acquire mut semaphore before modifying the shared vector
    worstCaseReaderFairRW.push_back(worstCase); // Store worst-case waiting time
    sem_post(&mut); // Release mut semaphore after modifying the shared vector
}

void writer(int id, int kw, int muCS, int muRem, vector<long long>& entryTimes, ofstream& logFile) {
    long long worstCase = numeric_limits<long long>::min(); // Initialize to minimum possible value
    for (int i = 0; i < kw; ++i) {
        auto reqTime = getSysTime();
        sem_wait(&mut);
        logEvent(logFile, to_string(i + 1) + "st CS request by Writer Thread " + to_string(id));
        sem_post(&mut);
        sem_wait(&serviceQueue);  // Wait in line to be serviced
        sem_wait(&resource);      // Request exclusive access to resource
        sem_post(&serviceQueue);  // Let next in line be serviced

        auto enterTime = getSysTime();
        sem_wait(&mut);
        logEvent(logFile, to_string(i + 1) + "st CS Entry by Writer Thread " + to_string(id));
        sem_post(&mut);

        // Calculate entry time and store it
        entryTimes.push_back(enterTime - reqTime);

        // Calculate worst-case waiting time
        long long waitingTime = enterTime - reqTime;
        if (waitingTime > worstCase) {
            worstCase = waitingTime;
        }

        // Simulate writing in CS
        this_thread::sleep_for(chrono::milliseconds(rand() % muCS));

        sem_post(&resource);  // Release resource access for next reader/writer

        auto exitTime = getSysTime();
        sem_wait(&mut);
        logEvent(logFile, to_string(i + 1) + "st CS Exit by Writer Thread " + to_string(id));
        sem_post(&mut);

        // Simulate remaining time outside CS
        this_thread::sleep_for(chrono::milliseconds(rand() % muRem));
    }
    sem_wait(&mut); // Acquire mut semaphore before modifying the shared vector
    worstCaseWriterFairRW.push_back(worstCase); // Store worst-case waiting time
    sem_post(&mut); // Release mut semaphore after modifying the shared vector
}

long long getSysTime() {
    return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
}

string formatTime(long long timestamp) {
    auto time = chrono::system_clock::to_time_t(chrono::system_clock::time_point(chrono::milliseconds(timestamp)));
    stringstream ss;
    ss << put_time(localtime(&time), "%T");

    // Extract day of the month
    int day = localtime(&time)->tm_mday;

    // Correct the ordinal suffix
    if (day >= 11 && day <= 13) {
        ss << " " << day << "th";
    } else {
        switch (day % 10) {
            case 1:  ss << " " << day << "st"; break;
            case 2:  ss << " " << day << "nd"; break;
            case 3:  ss << " " << day << "rd"; break;
            default: ss << " " << day << "th"; break;
        }
    }

    return ss.str();
}

void logEvent(ofstream& logFile, const string& event) {
    logFile << formatTime(getSysTime()) << " - " << event << endl;
}

void writeAverageTimeToFile(ofstream& avgTimeFile, const vector<long long>& entryTimes, const string& algorithm) {
    long long totalEntryTime = accumulate(entryTimes.begin(), entryTimes.end(), 0);
    double avgTime = static_cast<double>(totalEntryTime) / entryTimes.size();
    avgTimeFile << "Average time for " << algorithm << ": " << avgTime << " milliseconds" << endl;
}

void writeWorstCaseTimesToFile(ofstream& avgTimeFile, const vector<long long>& worstCaseTimes, const string& algorithm) {
    if (!worstCaseTimes.empty()) {
        long long worstCase = *max_element(worstCaseTimes.begin(), worstCaseTimes.end());
        avgTimeFile << "Worst-case time for " << algorithm << ": " << worstCase << " milliseconds" << endl;
    } else {
        avgTimeFile << "No worst-case time available for " << algorithm << endl;
    }
}
