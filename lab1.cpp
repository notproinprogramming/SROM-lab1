#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace std;

#define system_size 64 // 2048 біт / 32 = 64 слів
#define WORD_BITS 32
#define WORD_MASK 0xFFFFFFFFu

class BigInt {
    public:
        uint32_t a[system_size];

        BigInt() {
            for (int i = 0; i < system_size; i++)
                a[i] = 0;
        }

        BigInt(vector<uint32_t> bits) {
            for (int i = 0; i < system_size; i++)
                a[i] = 0;

            if (bits.size() % 2 != 0) {
                cerr << "ERROR: vector must be (index, value, index, value, "
                        "...)\n";
                return;
            }

            for (int i = 0; i < bits.size(); i += 2) {
                uint32_t pos = bits[i];
                uint32_t val = bits[i + 1];

                if (pos >= system_size) {
                    cerr << "ERROR: position " << pos << " out of range\n";
                    continue;
                }

                a[pos] = val;
            }
        }

        void Print() const {
            int i = system_size - 1;
            while (i > 0 && a[i] == 0)
                i--;

            cout << hex << a[i];
            for (i = i - 1; i >= 0; i--) {
                cout << "-" << setw(8) << setfill('0') << hex << a[i];
            }
            cout << dec << endl;
        }

        BigInt operator+(const BigInt &B) const {
            BigInt C;
            uint64_t carry = 0;

            for (int i = 0; i < system_size; i++) {
                uint64_t temp = (uint64_t)a[i] + B.a[i] + carry;
                C.a[i] = (uint32_t)(temp & WORD_MASK);
                carry = temp >> WORD_BITS;
            }

            return C;
        }

        BigInt operator-(const BigInt &B) const {
            BigInt C;
            uint64_t borrow = 0;

            for (int i = 0; i < system_size; i++) {
                int64_t temp = (int64_t)a[i] - B.a[i] - borrow;

                if (temp >= 0) {
                    C.a[i] = (uint32_t)temp;
                    borrow = 0;
                } else {
                    C.a[i] = (uint32_t)(temp + (1ull << WORD_BITS));
                    borrow = 1;
                }
            }

            return C;
        }

        int Cmp(const BigInt &B) const {
            for (int i = system_size - 1; i >= 0; i--) {
                if (a[i] > B.a[i])
                    return 1;
                if (a[i] < B.a[i])
                    return -1;
            }
            return 0;
        }
        // повертає 1  якщо A > B
        //           -1 якщо A < B
        //            0 якщо =

        bool operator<(const BigInt &B) const { return Cmp(B) < 0; }
        bool operator>(const BigInt &B) const { return Cmp(B) > 0; }
        bool operator==(const BigInt &B) const { return Cmp(B) == 0; }
};

int main() {
    BigInt A = BigInt();
    A.Print();

    vector<uint32_t> Bbits{62, 0xA, 0, 0x1};
    BigInt B = BigInt(Bbits);
    B.Print();
}
