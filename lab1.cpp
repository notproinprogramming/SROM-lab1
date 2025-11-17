#include <cstdint>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <vector>
using namespace std;

#define system_size 64 // 2048 / 32 = 64

class BigInt {
    public:
        uint32_t binary[system_size];
        int first_bit_position;

        BigInt() {
            for (int i = 0; i < system_size; i++)
                binary[i] = 0; // найстарший біт - нульовий, а наймолодший -
                               // system_size-1
            first_bit_position = -1;
        }

        BigInt(vector<uint32_t> bits) { // вектор виду (порядок біта, число,
                                        // порядок біта, число, ...)
            if (bits.size() <= 128) {
                if (bits.size() % 2 != 0) {
                    cerr << "ERROR: vector format entered incorrectly, SHOULD "
                            "BE "
                            "(bit position from 0 to "
                         << system_size - 1
                         << " , int32, bit position, int32,...)\n";
                } else if (bits.size() % 2 == 0) {
                    for (int i = 0; i < system_size; i++) // BigInt()
                        binary[i] = 0;
                    first_bit_position = bits[0];
                    for (int i = 0; i < bits.size(); i = i + 2) { // по позиціям
                        if (bits[i] < 64) {
                            binary[bits[i]] = bits[i + 1];
                        }
                        if (bits[i] < first_bit_position) {
                            first_bit_position = bits[i];
                        }
                    }
                }
            }
        }

        void Print() {
            if (first_bit_position == -1) {
                cout << 0 << endl;
                return;
            }

            cout << hex << binary[first_bit_position];

            for (int i = first_bit_position + 1; i < system_size; i++) {
                cout << "-";
                cout << setw(8) << setfill('0') << hex << binary[i];
            }

            cout << endl;

            cout << "First not 0 bit position = " << dec << first_bit_position
                 << endl;
        }
};

int main() {
    BigInt A = BigInt();
    A.Print();

    vector<uint32_t> Bbits{62, 0xA, 0, 0x1};
    BigInt B = BigInt(Bbits);
    B.Print();
}
