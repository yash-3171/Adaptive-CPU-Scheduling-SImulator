
#include <iostream>
#include <vector>
#include <deque>
#include <string>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <climits>

using namespace std;

// ============================================================
// 1. PROCESS CLASS
// ============================================================

class Process {
public:
    int id;
    string name;

    int arrivalTime;
    int burstTime;
    int priority;

    int remainingTime;
    int firstStartTime;
    int completionTime;

    Process(int processId, string processName,
            int arrival, int burst, int processPriority) {
        id = processId;
        name = processName;
        arrivalTime = arrival;
        burstTime = burst;
        priority = processPriority;

        remainingTime = burst;
        firstStartTime = -1;
        completionTime = -1;
    }

    int getTurnaroundTime() const {
        return completionTime - arrivalTime;
    }

    int getWaitingTime() const {
        return getTurnaroundTime() - burstTime;
    }

    int getResponseTime() const {
        return firstStartTime - arrivalTime;
    }
};

// ============================================================
// 2. SCHEDULING ALGORITHM TYPES
// ============================================================

enum class Algorithm {
    FCFS,
    SJF,
    ROUND_ROBIN,
    PRIORITY,
    ADAPTIVE
};

string algorithmName(Algorithm algorithm) {
    switch (algorithm) {
        case Algorithm::FCFS:
            return "FCFS";
        case Algorithm::SJF:
            return "Non-preemptive SJF";
        case Algorithm::ROUND_ROBIN:
            return "Round Robin";
        case Algorithm::PRIORITY:
            return "Priority Scheduling";
        case Algorithm::ADAPTIVE:
            return "Adaptive Scheduling";
    }

    return "Unknown";
}

// ============================================================
// 3. GANTT CHART SEGMENT
// ============================================================

class GanttSegment {
public:
    int processId;
    int startTime;
    int endTime;

    GanttSegment(int pid, int start, int end) {
        processId = pid;
        startTime = start;
        endTime = end;
    }
};

// ============================================================
// 4. SIMULATION RESULT
// ============================================================

class SimulationResult {
public:
    Algorithm algorithm;

    vector<Process> processes;
    vector<GanttSegment> ganttChart;

    int totalTime;
    int contextSwitches;
    int starvationThreshold;

    double averageWaitingTime;
    double averageTurnaroundTime;
    double averageResponseTime;
    double cpuUtilization;
    double throughput;

    string decisionExplanation;

    SimulationResult() {
        totalTime = 0;
        contextSwitches = 0;
        starvationThreshold = 10;

        averageWaitingTime = 0;
        averageTurnaroundTime = 0;
        averageResponseTime = 0;
        cpuUtilization = 0;
        throughput = 0;
    }
};

// ============================================================
// 5. ADAPTIVE SCHEDULER
// ============================================================

class AdaptiveScheduler {
public:
    // Lower numbers mean higher priority.
    // Adaptive policy chooses one algorithm for the workload.
    Algorithm chooseAlgorithm(
        const vector<Process>& processes,
        string& explanation
    ) {
        int n = static_cast<int>(processes.size());

        if (n == 0) {
            explanation = "No processes were supplied.";
            return Algorithm::FCFS;
        }

        int totalBurst = 0;
        int shortestBurst = INT_MAX;
        int longestBurst = 0;

        for (int i = 0; i < n; i++) {
            totalBurst += processes[i].burstTime;

            shortestBurst = min(
                shortestBurst, processes[i].burstTime
            );

            longestBurst = max(
                longestBurst, processes[i].burstTime
            );
        }

        double averageBurst =
            static_cast<double>(totalBurst) / n;

        // A larger or varied workload benefits from time sharing.
        if (n >= 5 || longestBurst - shortestBurst >= 5) {
            explanation =
                "Selected Round Robin because the workload has "
                "many processes or a large burst-time variation. "
                "Time sharing improves responsiveness.";

            return Algorithm::ROUND_ROBIN;
        }

        // For small workloads with short jobs, SJF reduces
        // average waiting time when burst lengths are known.
        if (averageBurst <= 4.0) {
            explanation =
                "Selected SJF because this is a small workload "
                "with short average CPU bursts.";

            return Algorithm::SJF;
        }

        explanation =
            "Selected FCFS because the workload is relatively "
            "small and does not strongly favor time sharing "
            "or short-job-first scheduling.";

        return Algorithm::FCFS;
    }
};

// ============================================================
// 6. CPU SCHEDULER / SIMULATION ENGINE
// ============================================================

class SimulationEngine {
private:
    AdaptiveScheduler adaptiveScheduler;

    static bool isNonPreemptive(Algorithm algorithm) {
        return algorithm == Algorithm::FCFS ||
               algorithm == Algorithm::SJF ||
               algorithm == Algorithm::PRIORITY;
    }

    static void addGanttSegment(
        vector<GanttSegment>& chart,
        int processId,
        int start,
        int end
    ) {
        if (!chart.empty() &&
            chart.back().processId == processId &&
            chart.back().endTime == start) {

            chart.back().endTime = end;
        } else {
            chart.push_back(
                GanttSegment(processId, start, end)
            );
        }
    }

    int selectNonPreemptiveProcess(
        const vector<Process>& processes,
        Algorithm algorithm,
        int currentTime
    ) {
        int selected = -1;

        for (int i = 0; i < static_cast<int>(processes.size()); i++) {
            const Process& p = processes[i];

            if (p.arrivalTime > currentTime ||
                p.remainingTime <= 0) {
                continue;
            }

            if (selected == -1) {
                selected = i;
                continue;
            }

            const Process& best = processes[selected];

            if (algorithm == Algorithm::FCFS) {
                if (p.arrivalTime < best.arrivalTime ||
                    (p.arrivalTime == best.arrivalTime &&
                     p.id < best.id)) {
                    selected = i;
                }
            } else if (algorithm == Algorithm::SJF) {
                if (p.burstTime < best.burstTime ||
                    (p.burstTime == best.burstTime &&
                     p.arrivalTime < best.arrivalTime) ||
                    (p.burstTime == best.burstTime &&
                     p.arrivalTime == best.arrivalTime &&
                     p.id < best.id)) {
                    selected = i;
                }
            } else if (algorithm == Algorithm::PRIORITY) {
                if (p.priority < best.priority ||
                    (p.priority == best.priority &&
                     p.arrivalTime < best.arrivalTime) ||
                    (p.priority == best.priority &&
                     p.arrivalTime == best.arrivalTime &&
                     p.id < best.id)) {
                    selected = i;
                }
            }
        }

        return selected;
    }

    void calculateMetrics(SimulationResult& result) {
        int n = static_cast<int>(result.processes.size());

        if (n == 0) {
            return;
        }

        double totalWaiting = 0;
        double totalTurnaround = 0;
        double totalResponse = 0;
        int totalBurst = 0;

        for (int i = 0; i < n; i++) {
            const Process& p = result.processes[i];

            totalWaiting += p.getWaitingTime();
            totalTurnaround += p.getTurnaroundTime();
            totalResponse += p.getResponseTime();
            totalBurst += p.burstTime;
        }

        result.averageWaitingTime = totalWaiting / n;
        result.averageTurnaroundTime = totalTurnaround / n;
        result.averageResponseTime = totalResponse / n;

        if (result.totalTime > 0) {
            result.cpuUtilization =
                100.0 * totalBurst / result.totalTime;

            result.throughput =
                static_cast<double>(n) / result.totalTime;
        }

        // Count switches between different running processes.
        // Idle-to-process transitions are not counted.
        for (int i = 1;
             i < static_cast<int>(result.ganttChart.size());
             i++) {

            int previous =
                result.ganttChart[i - 1].processId;

            int current =
                result.ganttChart[i].processId;

            if (previous != -1 &&
                current != -1 &&
                previous != current) {
                result.contextSwitches++;
            }
        }
    }

public:
    SimulationResult run(
        const vector<Process>& inputProcesses,
        Algorithm requestedAlgorithm,
        int quantum,
        int starvationThreshold
    ) {
        SimulationResult result;

        result.processes = inputProcesses;
        result.algorithm = requestedAlgorithm;
        result.starvationThreshold = starvationThreshold;

        if (requestedAlgorithm == Algorithm::ADAPTIVE) {
            result.algorithm =
                adaptiveScheduler.chooseAlgorithm(
                    inputProcesses,
                    result.decisionExplanation
                );
        } else {
            result.decisionExplanation =
                "The user explicitly selected " +
                algorithmName(requestedAlgorithm) + ".";
        }

        Algorithm algorithm = result.algorithm;

        // Sort process indexes by arrival time for Round Robin.
        vector<int> arrivalOrder(result.processes.size());

        for (int i = 0;
             i < static_cast<int>(arrivalOrder.size());
             i++) {
            arrivalOrder[i] = i;
        }

        sort(
            arrivalOrder.begin(),
            arrivalOrder.end(),
            [&result](int a, int b) {
                if (result.processes[a].arrivalTime !=
                    result.processes[b].arrivalTime) {
                    return result.processes[a].arrivalTime <
                           result.processes[b].arrivalTime;
                }

                return result.processes[a].id <
                       result.processes[b].id;
            }
        );

        deque<int> readyQueue;

        int nextArrival = 0;
        int currentProcess = -1;
        int quantumRemaining = quantum;
        int completed = 0;
        int currentTime = 0;

        int n = static_cast<int>(result.processes.size());

        while (completed < n) {
            // Add processes that have arrived by this time.
            if (algorithm == Algorithm::ROUND_ROBIN) {
                while (nextArrival < n &&
                       result.processes[
                           arrivalOrder[nextArrival]
                       ].arrivalTime <= currentTime) {

                    readyQueue.push_back(
                        arrivalOrder[nextArrival]
                    );

                    nextArrival++;
                }
            }

            // Round Robin: put an unfinished process at
            // the back of the queue when its quantum expires.
            if (algorithm == Algorithm::ROUND_ROBIN &&
                currentProcess != -1 &&
                quantumRemaining == 0) {

                if (result.processes[
                        currentProcess
                    ].remainingTime > 0) {

                    readyQueue.push_back(currentProcess);
                }

                currentProcess = -1;
                quantumRemaining = quantum;
            }

            // A completed process releases the CPU.
            if (currentProcess != -1 &&
                result.processes[
                    currentProcess
                ].remainingTime == 0) {

                currentProcess = -1;
                quantumRemaining = quantum;
            }

            // Select the next process if the CPU is free.
            if (currentProcess == -1) {
                if (algorithm == Algorithm::ROUND_ROBIN) {
                    if (!readyQueue.empty()) {
                        currentProcess = readyQueue.front();
                        readyQueue.pop_front();
                    }
                } else {
                    currentProcess =
                        selectNonPreemptiveProcess(
                            result.processes,
                            algorithm,
                            currentTime
                        );
                }
            }

            // No process is ready: CPU remains idle.
            if (currentProcess == -1) {
                addGanttSegment(
                    result.ganttChart,
                    -1,
                    currentTime,
                    currentTime + 1
                );

                currentTime++;
                continue;
            }

            Process& p = result.processes[currentProcess];

            if (p.firstStartTime == -1) {
                p.firstStartTime = currentTime;
            }

            // Execute the selected process for one time unit.
            addGanttSegment(
                result.ganttChart,
                p.id,
                currentTime,
                currentTime + 1
            );

            p.remainingTime--;
            currentTime++;

            if (algorithm == Algorithm::ROUND_ROBIN) {
                quantumRemaining--;
            }

            if (p.remainingTime == 0) {
                p.completionTime = currentTime;
                completed++;

                currentProcess = -1;
                quantumRemaining = quantum;
            }
        }

        result.totalTime = currentTime;

        calculateMetrics(result);

        return result;
    }
};

// ============================================================
// 7. DISPLAY / REPORTING
// ============================================================

class PerformanceAnalyzer {
public:
    static void printGanttChart(
        const SimulationResult& result
    ) {
        cout << "\nGANTT CHART\n";
        cout << "-----------------------------------------------\n";

        for (int i = 0;
             i < static_cast<int>(result.ganttChart.size());
             i++) {

            const GanttSegment& segment =
                result.ganttChart[i];

            if (segment.processId == -1) {
                cout << "| IDLE ";
            } else {
                cout << "| P" << segment.processId << " ";
            }
        }

        cout << "|\n";

        if (!result.ganttChart.empty()) {
            cout << result.ganttChart[0].startTime;

            for (int i = 0;
                 i < static_cast<int>(result.ganttChart.size());
                 i++) {

                cout << setw(7)
                     << result.ganttChart[i].endTime;
            }

            cout << "\n";
        }
    }

    static void printProcessTable(
        const SimulationResult& result
    ) {
        cout << "\nPROCESS DETAILS\n";

        cout << left
             << setw(8)  << "PID"
             << setw(12) << "Arrival"
             << setw(10) << "Burst"
             << setw(10) << "Priority"
             << setw(10) << "Start"
             << setw(12) << "Complete"
             << setw(12) << "Turnaround"
             << setw(10) << "Waiting"
             << setw(10) << "Response"
             << "\n";

        cout << string(94, '-') << "\n";

        for (int i = 0;
             i < static_cast<int>(result.processes.size());
             i++) {

            const Process& p = result.processes[i];

            cout << left
                 << setw(8)  << p.id
                 << setw(12) << p.arrivalTime
                 << setw(10) << p.burstTime
                 << setw(10) << p.priority
                 << setw(10) << p.firstStartTime
                 << setw(12) << p.completionTime
                 << setw(12) << p.getTurnaroundTime()
                 << setw(10) << p.getWaitingTime()
                 << setw(10) << p.getResponseTime()
                 << "\n";
        }
    }

    static void printMetrics(
        const SimulationResult& result
    ) {
        cout << fixed << setprecision(2);

        cout << "\nPERFORMANCE REPORT\n";
        cout << "-----------------------------------------------\n";

        cout << "Algorithm used: "
             << algorithmName(result.algorithm) << "\n";

        cout << "Decision explanation: "
             << result.decisionExplanation << "\n";

        cout << "Average waiting time: "
             << result.averageWaitingTime << "\n";

        cout << "Average turnaround time: "
             << result.averageTurnaroundTime << "\n";

        cout << "Average response time: "
             << result.averageResponseTime << "\n";

        cout << "CPU utilization: "
             << result.cpuUtilization << "%\n";

        cout << "Throughput: "
             << result.throughput
             << " processes per time unit\n";

        cout << "Context switches: "
             << result.contextSwitches << "\n";

        cout << "Total simulation time: "
             << result.totalTime << "\n";

        cout << "\nSTARVATION / LONG-WAIT CHECK\n";

        bool found = false;

        for (int i = 0;
             i < static_cast<int>(result.processes.size());
             i++) {

            const Process& p = result.processes[i];

            if (p.getWaitingTime() >
                result.starvationThreshold) {

                cout << "Warning: P" << p.id
                     << " (" << p.name << ") waited "
                     << p.getWaitingTime()
                     << " time units. This exceeds the threshold.\n";

                found = true;
            }
        }

        if (!found) {
            cout << "No process exceeded the configured "
                 << "long-wait threshold of "
                 << result.starvationThreshold
                 << " time units.\n";
        }

        cout << "\nNote: Long waiting is a starvation-risk "
             << "indicator, not proof of indefinite starvation.\n";
    }

    static void printFullReport(
        const SimulationResult& result
    ) {
        printGanttChart(result);
        printProcessTable(result);
        printMetrics(result);
    }

    static void printComparison(
        const vector<SimulationResult>& results
    ) {
        cout << "\nALGORITHM COMPARISON\n";

        cout << left
             << setw(24) << "Algorithm"
             << setw(14) << "Avg Waiting"
             << setw(16) << "Avg Turnaround"
             << setw(15) << "Avg Response"
             << setw(12) << "CPU Util %"
             << setw(12) << "Switches"
             << "\n";

        cout << string(93, '-') << "\n";

        cout << fixed << setprecision(2);

        for (int i = 0;
             i < static_cast<int>(results.size());
             i++) {

            const SimulationResult& r = results[i];

            cout << left
                 << setw(24) << algorithmName(r.algorithm)
                 << setw(14) << r.averageWaitingTime
                 << setw(16) << r.averageTurnaroundTime
                 << setw(15) << r.averageResponseTime
                 << setw(12) << r.cpuUtilization
                 << setw(12) << r.contextSwitches
                 << "\n";
        }

        cout << "\nLower waiting/response times are not always "
             << "the only goal. Round Robin may improve "
             << "responsiveness while increasing context switches.\n";
    }
};

// ============================================================
// 8. INPUT HELPERS
// ============================================================

int readInteger(string prompt, int minimum, int maximum) {
    int value;

    while (true) {
        cout << prompt;

        if (cin >> value && value >= minimum &&
            value <= maximum) {
            return value;
        }

        cout << "Invalid input. Enter a number from "
             << minimum << " to " << maximum << ".\n";

        cin.clear();
        cin.ignore(
            numeric_limits<streamsize>::max(),
            '\n'
        );
    }
}

vector<Process> readProcesses() {
    int n = readInteger(
        "\nEnter number of processes (1-100): ",
        1, 100
    );

    vector<Process> processes;

    cout << "\nEnter arrival time, burst time and priority "
         << "for each process.\n";

    cout << "Priority: 1 is highest; larger numbers are lower.\n";
    cout << "Example: 0 5 2\n\n";

    for (int i = 0; i < n; i++) {
        int arrival, burst, priority;

        cout << "Process P" << i + 1 << ":\n";

        arrival = readInteger("  Arrival time (0-10000): ",
                              0, 10000);

        burst = readInteger("  Burst time (1-10000): ",
                            1, 10000);

        priority = readInteger("  Priority (1-100): ",
                               1, 100);

        string name = "P" + to_string(i + 1);

        processes.push_back(
            Process(i + 1, name, arrival, burst, priority)
        );
    }

    return processes;
}

Algorithm readAlgorithm() {
    cout << "\nChoose scheduling algorithm:\n";
    cout << "1. FCFS\n";
    cout << "2. Non-preemptive SJF\n";
    cout << "3. Round Robin\n";
    cout << "4. Priority Scheduling\n";
    cout << "5. Adaptive Scheduling\n";

    int choice = readInteger("Choice: ", 1, 5);

    switch (choice) {
        case 1:
            return Algorithm::FCFS;
        case 2:
            return Algorithm::SJF;
        case 3:
            return Algorithm::ROUND_ROBIN;
        case 4:
            return Algorithm::PRIORITY;
        default:
            return Algorithm::ADAPTIVE;
    }
}

// ============================================================
// 9. DEMO WORKLOAD
// ============================================================

vector<Process> createDemoProcesses() {
    vector<Process> processes;

    processes.push_back(Process(1, "P1", 0, 7, 2));
    processes.push_back(Process(2, "P2", 2, 4, 1));
    processes.push_back(Process(3, "P3", 3, 1, 3));
    processes.push_back(Process(4, "P4", 5, 4, 2));
    processes.push_back(Process(5, "P5", 6, 2, 1));

    return processes;
}

// ============================================================
// 10. MAIN MENU
// ============================================================

int main() {
    SimulationEngine engine;

    while (true) {
        cout << "\n";
        cout << "===============================================\n";
        cout << "      ADAPTIVE CPU SCHEDULING SIMULATOR\n";
        cout << "===============================================\n";

        cout << "1. Run one scheduling algorithm\n";
        cout << "2. Compare all algorithms on a workload\n";
        cout << "3. Run built-in demo\n";
        cout << "0. Exit\n";

        int choice = readInteger("Enter choice: ", 0, 3);

        if (choice == 0) {
            cout << "Exiting simulator. Goodbye!\n";
            break;
        }

        vector<Process> processes;

        if (choice == 3) {
            processes = createDemoProcesses();
            cout << "\nUsing built-in demo workload.\n";
        } else {
            processes = readProcesses();
        }

        int quantum = readInteger(
            "\nEnter Round Robin time quantum (1-100): ",
            1, 100
        );

        int starvationThreshold = readInteger(
            "Enter long-wait warning threshold (1-100000): ",
            1, 100000
        );

        if (choice == 1) {
            Algorithm algorithm = readAlgorithm();

            SimulationResult result = engine.run(
                processes,
                algorithm,
                quantum,
                starvationThreshold
            );

            PerformanceAnalyzer::printFullReport(result);
        } else {
            vector<SimulationResult> results;

            results.push_back(
                engine.run(processes, Algorithm::FCFS,
                           quantum, starvationThreshold)
            );

            results.push_back(
                engine.run(processes, Algorithm::SJF,
                           quantum, starvationThreshold)
            );

            results.push_back(
                engine.run(processes, Algorithm::ROUND_ROBIN,
                           quantum, starvationThreshold)
            );

            results.push_back(
                engine.run(processes, Algorithm::PRIORITY,
                           quantum, starvationThreshold)
            );

            results.push_back(
                engine.run(processes, Algorithm::ADAPTIVE,
                           quantum, starvationThreshold)
            );

            PerformanceAnalyzer::printComparison(results);

            cout << "\nTo inspect the Gantt chart and process-level "
                 << "metrics, choose option 1 and run an algorithm.\n";
        }
    }

    return 0;
}