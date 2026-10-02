/*
    EECS 348 Assignment 2
    Author: Zoey Spies
    KUID: 3136594
    Creation Date: 9/17/2026
    Revision date: 9/17/2026
    Purpose: Sort emails in order of priorty with the ability to call the most important emails off the heap as needed
    Collaborators: Copilot, Claude, EECS 348 notes and slides, EECS 388 notes and slides
*/

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

// ---------------------------------------------------------------
// Email: one message in the CEO's inbox plus its priority info.
// ---------------------------------------------------------------
class Email {
private:
    string sender;     // sender category, e.g. "Boss"
    string subject;    // subject line (may contain spaces)
    string date;       // date as given, MM-DD-YYYY
    int rank;          // numeric priority from sender category
    long dateValue;    // date encoded as YYYYMMDD for comparison

    // Map sender category to a rank (higher = read first).
    static int rankFor(const string &s) {
        if (s == "Boss") return 5;
        if (s == "Subordinate") return 4;
        if (s == "Peer") return 3;
        if (s == "ImportantPerson") return 2;
        if (s == "OtherPerson") return 1;
        return 0; // unknown category: lowest
    }

    // Convert MM-DD-YYYY to YYYYMMDD so dates compare as integers.
    static long valueFor(const string &d) {
        if (d.size() < 10) return 0; // malformed: treat as oldest
        int month = stoi(d.substr(0, 2));
        int day = stoi(d.substr(3, 2));
        int year = stoi(d.substr(6, 4));
        return year * 10000L + month * 100L + day;
    }

public:
    Email() : rank(0), dateValue(0) {} // default needed for array allocation

    Email(const string &s, const string &subj, const string &d)
        : sender(s), subject(subj), date(d),
          rank(rankFor(s)), dateValue(valueFor(d)) {}

    // True if this email should be read before 'other'.
    bool higherPriorityThan(const Email &other) const {
        if (rank != other.rank) return rank > other.rank; // category first
        return dateValue > other.dateValue;               // then newest first
    }

    void display() const {
        cout << "Next email:\n"
             << "Sender: " << sender << "\n"
             << "Subject: " << subject << "\n"
             << "Date: " << date << "\n";
    }
};

// ---------------------------------------------------------------
// MaxHeap: list-based (dynamic array) binary max heap of Emails.
// ---------------------------------------------------------------
class MaxHeap {
private:
    Email *items;   // underlying array
    int count;      // elements stored
    int capacity;   // allocated slots

    // Double the array when full.
    void grow() {
        int newCap = capacity * 2;
        Email *bigger = new Email[newCap];
        for (int i = 0; i < count; i++) bigger[i] = items[i];
        delete[] items;
        items = bigger;
        capacity = newCap;
    }

    void swapItems(int i, int j) {
        Email tmp = items[i];
        items[i] = items[j];
        items[j] = tmp;
    }

    // Move element at i up until its parent outranks it.
    void siftUp(int i) {
        while (i > 0) {
            int parent = (i - 1) / 2;
            if (!items[i].higherPriorityThan(items[parent])) break;
            swapItems(i, parent);
            i = parent;
        }
    }

    // Move element at i down until both children rank lower.
    void siftDown(int i) {
        while (true) {
            int left = 2 * i + 1;
            int right = 2 * i + 2;
            int best = i;
            if (left < count && items[left].higherPriorityThan(items[best])) best = left;
            if (right < count && items[right].higherPriorityThan(items[best])) best = right;
            if (best == i) break;
            swapItems(i, best);
            i = best;
        }
    }

public:
    MaxHeap() : items(new Email[8]), count(0), capacity(8) {}
    ~MaxHeap() { delete[] items; }

    // Copying would double-delete the array, so disable it.
    MaxHeap(const MaxHeap &) = delete;
    MaxHeap &operator=(const MaxHeap &) = delete;

    int size() const { return count; }
    bool empty() const { return count == 0; }

    void insert(const Email &e) {
        if (count == capacity) grow();
        items[count] = e;
        siftUp(count);
        count++;
    }

    // Highest-priority email without removing it (call only if not empty).
    const Email &peek() const { return items[0]; }

    // Remove the highest-priority email. Returns false if empty.
    bool extractMax() {
        if (count == 0) return false;
        count--;
        items[0] = items[count];
        if (count > 0) siftDown(0);
        return true;
    }
};

// ---------------------------------------------------------------
// CEOInbox: parses commands and drives the heap.
// ---------------------------------------------------------------
class CEOInbox {
private:
    MaxHeap heap;

    void addEmail(const string &rest) {
        size_t c1 = rest.find(',');
        size_t c2 = (c1 == string::npos) ? string::npos : rest.find(',', c1 + 1);
        if (c2 == string::npos) {
            cerr << "Warning: malformed EMAIL line ignored\n";
            return;
        }
        heap.insert(Email(rest.substr(0, c1),
                          rest.substr(c1 + 1, c2 - c1 - 1),
                          rest.substr(c2 + 1)));
    }

public:
    void process(string line) {
        // Strip trailing CR/LF so Windows line endings don't break matching.
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();
        if (line.empty()) return;

        if (line.compare(0, 6, "EMAIL ") == 0) {
            addEmail(line.substr(6));
        } else if (line == "NEXT") {
            if (heap.empty()) cout << "No emails to read.\n";
            else heap.peek().display();
        } else if (line == "READ") {
            if (!heap.extractMax()) cout << "No emails to read.\n";
        } else if (line == "COUNT") {
            cout << "There are " << heap.size() << " emails to read.\n";
        } else {
            cerr << "Warning: unrecognized command ignored: " << line << "\n";
        }
    }
};

int main(int argc, char *argv[]) {
    istream *in = &cin;
    ifstream file;
    if (argc > 1) {
        file.open(argv[1]);
        if (!file) {
            cerr << "Error: could not open file " << argv[1] << "\n";
            return 1;
        }
        in = &file;
    }

    CEOInbox inbox;
    string line;
    while (getline(*in, line)) inbox.process(line);
    return 0;
}