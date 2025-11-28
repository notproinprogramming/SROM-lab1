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
            if (bits.size() % 2 != 0) {
                cerr << "ERROR: vector must be (index, value, index, value, "
                        "...)\n";
                return;
            }
            for (int i = 0; i < system_size; i++)
                a[i] = 0;

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

        BigInt(const std::string &hex_input) {
            // Очистити масив
            for (int i = 0; i < system_size; i++)
                a[i] = 0;

            // Копія рядка без "0x" і пробілів
            std::string hex = "";
            for (char c : hex_input) {
                if (c == ' ' || c == '\t')
                    continue;
                if (c == '0' && (hex.size() == 0))
                    continue;
                if ((c == 'x' || c == 'X') && hex.size() == 1 &&
                    hex[0] == '0') {
                    hex.clear();
                    continue;
                }
                hex.push_back(c);
            }

            if (hex.empty()) {
                return;
            }

            // одна hex-цифра = 4 біти
            const int total_bits = hex.size() * 4;

            // кількість 32-бітних слів, які потрібно заповнити
            int word_index = 0;
            uint32_t current_word = 0;
            int bits_filled = 0;

            // читаємо hex справа наліво (молодші → старші)
            for (int i = hex.size() - 1; i >= 0; i--) {
                char c = hex[i];
                uint32_t value;

                if (c >= '0' && c <= '9')
                    value = c - '0';
                else if (c >= 'a' && c <= 'f')
                    value = 10 + (c - 'a');
                else if (c >= 'A' && c <= 'F')
                    value = 10 + (c - 'A');
                else
                    continue;

                // додаємо 4 біти у 32-бітне слово
                current_word |= (value << bits_filled);
                bits_filled += 4;

                // якщо слово наповнилося — записуємо
                if (bits_filled == 32) {
                    if (word_index < system_size)
                        a[word_index] = current_word;
                    word_index++;

                    current_word = 0;
                    bits_filled = 0;
                }
            }

            // записуємо останнє неповне слово
            if (bits_filled > 0 && word_index < system_size)
                a[word_index] = current_word;

            // знайти позицію найстаршого біта
            int msw = word_index;
            if (bits_filled > 0)
                msw = word_index;
            else
                msw = word_index - 1;

            if (msw < 0) {
                return;
            }

            uint32_t top = a[msw];
            int highest_bit = 31;
            while (highest_bit >= 0 && ((top >> highest_bit) & 1) == 0)
                highest_bit--;
        }

        BigInt(const BigInt &A) {
            for (int i = system_size - 1; i >= 0; i--) {
                a[i] = A.a[i];
            }
        }

        BigInt &operator=(const BigInt &B) {
            if (this != &B) {
                for (int i = 0; i < system_size; i++)
                    this->a[i] = B.a[i];
            }
            return *this;
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
                C.a[i] =
                    (uint32_t)(temp &
                               WORD_MASK); // беремо залишок від ділення на
                                           // 0xFFFFFFFF, тобто нижні 32 біти
                carry = temp >> WORD_BITS; // переносимо temp на 32 біти, тобто
                                           // залишаємо тільки 33 біт
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
            if (borrow != 0) {
                cerr << "ERROR: subtraction resulted in negative number\n";
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

        BigInt LongMulOneDigit(const BigInt &A, uint32_t b) {
            BigInt C;
            uint64_t carry = 0;

            for (int i = 0; i < system_size; i++) {
                uint64_t temp = (uint64_t)A.a[i] * (uint64_t)b + carry;

                C.a[i] = (uint32_t)(temp & 0xFFFFFFFFu); // нижні 32 біти
                carry = temp >> 32;                      // перенос
            }

            return C;
        }

        void LongShiftDigitsToHigh(BigInt &X, int shift) {
            if (shift == 0)
                return;

            for (int i = system_size - 1; i >= shift; i--) {
                X.a[i] = X.a[i - shift];
            }
            for (int i = 0; i < shift; i++) {
                X.a[i] = 0;
            }
        }

        BigInt LongMul(const BigInt &A, const BigInt &B) {
            BigInt C;

            for (int i = 0; i < system_size; i++) {
                BigInt temp = LongMulOneDigit(A, B.a[i]);

                LongShiftDigitsToHigh(temp, i); // temp << (i*32)
                C = C + temp;
            }

            return C;
        }
        BigInt operator*(BigInt &B) {
            BigInt C;
            C = LongMul(*this, B);
            return C;
        }
        BigInt LongShiftBitsToHigh(const BigInt &X, int shiftBits) {
            BigInt R;

            int wordShift = shiftBits / 32;
            int bitShift = shiftBits % 32;

            // Зміщення на слова
            for (int i = system_size - 1; i >= wordShift; i--) {
                R.a[i] = X.a[i - wordShift];
            }
            for (int i = 0; i < wordShift; i++) {
                R.a[i] = 0;
            }

            // Додатково зміщення всередині слова
            if (bitShift > 0) {
                uint32_t carry = 0;
                for (int i = wordShift; i < system_size; i++) {
                    uint64_t v = ((uint64_t)R.a[i] << bitShift) | carry;
                    R.a[i] = (uint32_t)v;
                    carry = v >> 32;
                }
            }

            return R;
        }

        int BitLength(const BigInt &X) {
            for (int i = system_size - 1; i >= 0; i--) {
                if (X.a[i] != 0) {
                    return i * 32 + (32 - __builtin_clz(X.a[i]));
                }
            }
            return 0;
        }

        void LongDivMod(const BigInt &A, const BigInt &B, BigInt &Q,
                        BigInt &R) {
            Q = BigInt();
            R = A;

            int k = BitLength(B);

            while (!(R < B)) { // R >= B
                int t = BitLength(R);

                BigInt C = LongShiftBitsToHigh(B, t - k);

                if (R < C) { // якщо перелетіли
                    t = t - 1;
                    C = LongShiftBitsToHigh(B, t - k);
                }

                R = R - C; // R -= C

                // Q += 2^(t-k)
                int bit = t - k;
                Q.a[bit / 32] |= (1u << (bit % 32));
            }
        }

        int GetBit(int bitnum) const {
            if (bitnum < 0 || bitnum >= WORD_BITS * system_size) {
                return -1;
            }
            int intnum = bitnum / WORD_BITS;
            bitnum = bitnum % WORD_BITS;
            int bit = (a[intnum] >> bitnum) & 1;
            return bit;
        }

        BigInt Gorner(BigInt &A, BigInt &B) {
            BigInt C = BigInt("1");
            BigInt Acopy = A;
            for (int i = 0; i < BitLength(B); i++) {
                if (B.GetBit(i) == 1) {

                    C = C * Acopy;
                }
                Acopy = Acopy * Acopy;
            }
            return C;
        }

        BigInt Gorner(const BigInt &A, int b) {
            BigInt C = BigInt("1");
            BigInt Acopy = A;
            while (b > 0) {
                if (b & 1) {
                    C = C * Acopy;
                }
                Acopy = Acopy * Acopy;
                b = b >> 1;
            }
            return C;
        }
};

int main() {
    /*    BigInt A = BigInt();
        A.Print();

        vector<uint32_t> Bbits{62, 0xA, 0, 0x1};
        BigInt B = BigInt(Bbits);
        B.Print();
        BigInt C = BigInt();
        A = BigInt({0, 1});
        A.Print();
        int k = A > B;
        cout << k << endl;
        C = B - A;

        C.Print();

        int b = 5;
        BigInt D = BigInt();
        D = D.LongMulOneDigit(A, b);
        D.Print();
        A = BigInt({30, 7, 61, 6});
        D = D.LongMul(A, B);
        D.Print();
        cout << "==================================================\n";

    */

    BigInt A("aaaaffffff25534678765143534fff");
    BigInt B("15");

    cout << "A = ";
    A.Print();

    cout << "B = ";
    B.Print();

    BigInt S = A + B;
    cout << "A + B = ";
    S.Print();

    BigInt D = A - B;
    cout << "A - B = ";
    D.Print();

    BigInt M;
    M = M.LongMul(A, B);
    cout << "A * B = ";
    M.Print();

    BigInt Q, R;
    Q.LongDivMod(A, B, Q, R);
    cout << "A / B = ";
    Q.Print();
    cout << "A % B = ";
    R.Print();

    BigInt L;
    L = L.Gorner(A, B);
    cout << "A ^ B = ";
    L.Print();
    BigInt L1;
    L1 = L1.Gorner(A, 5);
    cout << "A ^ 5 = ";
    L1.Print();
    //  for (int i = 0; i < 64; i++)
    //    cout << M.GetBit(i) << endl;
    return 0;
}
