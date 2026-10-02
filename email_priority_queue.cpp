/*
    EECS 348 Assignment 3
    Author: Zoey Spies
    KUID: 3136594
    Creation Date: 10/1/2026
    Revision date: 10/1/2026
    Purpose: Sort emails in order of priorty with the ability to call the most important emails off the heap as needed
    Collaborators: Copilot, Claude, EECS 348 notes and slides, EECS 388 notes and slides
*/

#include <iostream> // cin, cout, cerr
#include <fstream> // ifstream for reading the test file
#include <string> // std::string
#include <utility> // std::move, std::swap

using namespace std; // avoid std:: prefix for cin, cout, cerr, string, move, swap

/* ------------------------------------------------------------------
   Email: one message in the CEO's inbox plus its priority information.
   Rank and date value are computed ONCE (in the constructor) so every
   heap comparison is just integer comparison.
   ------------------------------------------------------------------ */
class Email { // represents an email with a priority
private: // data members
    string sender;      // sender category, e.g. "Boss"
    string subject;     // subject line (may contain spaces, no commas)
    string date;        // date exactly as given, MM-DD-YYYY
    int    rank;        // numeric priority from sender category (higher = first)
    long   dateValue;   // date encoded as YYYYMMDD for integer comparison

    // Map a sender category to a rank. Unknown categories rank lowest (0).
    static int rankFor(const string &s) { // s is the sender category
        if (s == "Boss")            return 5; // highest priority
        if (s == "Subordinate")     return 4; // next highest
        if (s == "Peer")            return 3; // next highest
        if (s == "ImportantPerson") return 2; // next highest
        if (s == "OtherPerson")     return 1; // lowest priority
        return 0;
    }

    // Convert "MM-DD-YYYY" to YYYYMMDD to compare dates.
    // A malformed date returns 0 (treated as the oldest possible date).
    static long valueFor(const string &d) {
        if (d.size() != 10 || d[2] != '-' || d[5] != '-') return 0; // wrong shape
        for (size_t i = 0; i < d.size(); i++) { // check each character
            if (i == 2 || i == 5) continue; // skip the dashes
            if (d[i] < '0' || d[i] > '9') return 0; // non-digit found
        }
        long month = (d[0] - '0') * 10 + (d[1] - '0');     // MM
        long day   = (d[3] - '0') * 10 + (d[4] - '0');     // DD
        long year  = (d[6] - '0') * 1000 + (d[7] - '0') * 100
                   + (d[8] - '0') * 10   + (d[9] - '0');   // YYYY
        return year * 10000L + month * 100L + day; // YYYYMMDD
    }

public:
    Email() : rank(0), dateValue(0) {}  // default constructor (needed for arrays)

    Email(const string &s, const string &subj, const string &d) // constructor with sender, subject, date
        : sender(s), subject(subj), date(d), // compute rank and dateValue once
          rank(rankFor(s)), dateValue(valueFor(d)) {} // constructor with sender, subject, date

    // True if this email should be read BEFORE 'other':
    // higher category rank wins; on a tie the newer date wins.
    bool higherPriorityThan(const Email &other) const { // true if this email outranks the other
        if (rank != other.rank) return rank > other.rank; // higher rank wins
        return dateValue > other.dateValue; // newer date wins
    }

    // Print this email in the format required by the NEXT command.
    void display() const { // print the email in the required format
        cout << "Next email:\n" // expected output format for NEXT command
             << "Sender: "  << sender  << "\n" // each field on its own line
             << "Subject: " << subject << "\n" // subject may contain spaces, but no commas
             << "Date: "    << date    << "\n"; // date is printed exactly as given, MM-DD-YYYY
    }
};

/* ------------------------------------------------------------------
   MaxHeap: list-based (dynamic array) binary max-heap of Emails.
   Written from scratch; the array doubles when it fills up.
   ------------------------------------------------------------------ */
class MaxHeap { // max-heap of Email objects
private: // data members
    Email *items;    // underlying array; items[0] is the highest priority
    int    count;    // number of emails currently stored
    int    capacity; // number of slots allocated

    // Double the array when full, MOVING emails instead of copying strings.
    void grow() { // double the array size, moving emails to the new array
        int newCap = capacity * 2; // double the capacity
        Email *bigger = new Email[newCap]; // allocate new array
        // Move each email to the new array
        for (int i = 0; i < count; i++) bigger[i] = std::move(items[i]); // move each email to the new array
        delete[] items; // free the old array
        items = bigger; // point to the new array
        capacity = newCap; // update the capacity
    }

    // Swap two array slots (std::swap moves rather than copies strings).
    void swapItems(int i, int j) { std::swap(items[i], items[j]); } // swap two emails in the array

    // Move the element at index i up until its parent outranks it.
    void siftUp(int i) { // move the email at index i up the heap until it is outranked by its parent
        while (i > 0) { // while not at the root
            int parent = (i - 1) / 2; // compute the parent index
            if (!items[i].higherPriorityThan(items[parent])) break; // if the email is outranked by its parent, stop
            swapItems(i, parent); // swap the email with its parent
            i = parent; // move up to the parent index
        }
    }

    // Move the element at index i down until both children rank lower.
    void siftDown(int i) { // move the email at index i down the heap until it outranks both children
        while (true) { // loop until the email is in the correct position
            int left  = 2 * i + 1; // compute the left child index
            int right = 2 * i + 2; // compute the right child index
            int best  = i; // assume the current email is the best
            if (left  < count && items[left].higherPriorityThan(items[best]))  best = left; // if the left child outranks the best so far, update best
            else if (right < count && items[right].higherPriorityThan(items[best])) best = right; // if the right child outranks the best so far, update best
            else if (best == i) break; // if the current email outranks both children, stop
            swapItems(i, best); // swap the email with the best child
            i = best; // move down to the best child index
        }
    }

public: // public interface
    MaxHeap() : items(new Email[8]), count(0), capacity(8) {} // default constructor: allocate an array of 8 emails
    ~MaxHeap() { delete[] items; } // destructor: free the array

    // Copying would cause a double delete of the array, so it is disabled.
    MaxHeap(const MaxHeap &) = delete; // disable copy constructor
    MaxHeap &operator=(const MaxHeap &) = delete; // disable copy assignment operator

    // Get the number of emails in the heap.
    // O(1)
    int  size()  const { return count; } // return the number of emails in the heap
    bool empty() const { return count == 0; } // return true if the heap is empty

    // Insert an email: place at the end, then sift up. O(log n).
    void insert(Email e) { // insert an email into the heap
        if (count == capacity) grow(); // if the array is full, double its size
        items[count] = std::move(e); // place the email at the end of the array
        siftUp(count); // sift up the email to restore the heap property
        count++; // increment the count of emails in the heap
    }

    // Highest-priority email without removing it. O(1). Call only if not empty.
    const Email &peek() const { return items[0]; } // return a reference to the highest-priority email without removing it

    // Remove the highest-priority email. Returns false if the heap is empty.
    bool extractMax() { // remove the highest-priority email from the heap
        if (count == 0) return false; // if the heap is empty, return false
        count--; // decrement the count of emails in the heap
        items[0] = std::move(items[count]);  // last element becomes the root
        if (count > 0) siftDown(0); // sift down the new root to restore the heap property
        return true; // return true to indicate that an email was removed
    }
};

/* ------------------------------------------------------------------
   CEOInbox: parses command lines and drives the MaxHeap.
   ------------------------------------------------------------------ */
class CEOInbox { // manages the CEO's inbox using a max-heap of emails
private: // data members
    MaxHeap heap; // the max-heap of emails

    // Remove spaces, tabs, CR and LF from both ends of a string.
    static string trim(const string &s) { // remove whitespace from both ends of a string
        const string ws = " \t\r\n"; // whitespace characters to remove
        size_t start = s.find_first_not_of(ws); // find the first non-whitespace character
        if (start == string::npos) return "";         // all whitespace
        size_t end = s.find_last_not_of(ws); // find the last non-whitespace character
        return s.substr(start, end - start + 1); // return the trimmed string
    }

    // Parse "<category>,<subject>,<date>" and insert the email.
    void addEmail(const string &rest) { // parse the rest of the line and insert the email into the heap
        size_t c1 = rest.find(','); // find the first comma
        size_t c2 = (c1 == string::npos) ? string::npos : rest.find(',', c1 + 1); // find the second comma
        if (c2 == string::npos) {                     // fewer than two commas
            cerr << "Warning: malformed EMAIL line ignored\n"; // print a warning message
            return; // ignore this line
        }
        heap.insert(Email(trim(rest.substr(0, c1)), // extract the sender category
                          trim(rest.substr(c1 + 1, c2 - c1 - 1)), // extract the subject
                          trim(rest.substr(c2 + 1)))); // extract the date and insert the email into the heap
    }

public: // public interface
    // Handle one line of the input file.
    void process(const string &rawLine) { // process one line of input
        string line = trim(rawLine); // also strips Windows '\r'
        if (line.empty()) return; // skip blank lines
        // The first word of the line determines the command.
        if (line.compare(0, 6, "EMAIL ") == 0) { // add an email to the heap
            addEmail(line.substr(6));// parse the rest of the line and insert the email into the heap
        } else if (line == "NEXT") { // show the highest priority email
            // Show the highest priority email without removing it.
            if (heap.empty()) cout << "No emails to read.\n"; // if the heap is empty, print a message
            else heap.peek().display(); // show, do not remove
        } else if (line == "READ") { // remove the highest priority email
            if (!heap.extractMax()) cout << "No emails to read.\n"; // if the heap is empty, print a message
        } else if (line == "COUNT") { // show the number of emails in the heap
            cout << "There are " << heap.size() << " emails to read.\n"; // print the number of emails in the heap
        } else { // unrecognized command
            cerr << "Warning: unrecognized command ignored: " << line << "\n"; // print a warning message
        }
    }
};

/* ------------------------------------------------------------------
   main: read commands from argv[1] if given, otherwise from stdin.
   ------------------------------------------------------------------ */
int main(int argc, char *argv[]) {
    istream *in = &cin;      // default input is the keyboard / piped stdin
    ifstream file;           // used only when a filename is supplied

    if (argc > 1) { // if a filename is given on the command line, read from that file
        file.open(argv[1]); // open the file for reading
        if (!file) { // if the file could not be opened, print an error message and exit
            cerr << "Error: could not open file " << argv[1] << "\n"; // print an error message
            return 1; // exit with error code 1
        }
        in = &file; // redirect input to the file
    }

    CEOInbox inbox; // create an instance of the CEOInbox class to manage the inbox
    string line; // buffer for reading lines from the input
    while (getline(*in, line)) inbox.process(line);   // one command per line
    return 0; // exit with success
}
