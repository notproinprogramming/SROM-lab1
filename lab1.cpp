#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <ratio>
#include <string>
#include <vector>

using namespace std;

#define system_size 64 // 2048 біт / 32 = 64 слів
#define WORD_BITS 32
#define WORD_MASK 0xFFFFFFFFu

string DecimalToHex(string decimal) {
    if (decimal.empty())
        return "";
    if (decimal == "0")
        return "0";

    string hex = "";
    string current = decimal;

    while (current != "" && current != "0") {
        int remainder = 0;
        string quotient = "";
        bool leading_zeros = true;
        for (char c : current) {
            int digit = c - '0';
            int val = remainder * 10 + digit;
            int q = val / 16;
            remainder = val % 16;
            if (q != 0)
                leading_zeros = false;
            if (!leading_zeros) {
                quotient += (char)(q + '0');
            }
        }

        if (remainder < 10)
            hex += (char)(remainder + '0');
        else
            hex += (char)(remainder - 10 + 'A');
        current = quotient;
    }

    reverse(hex.begin(), hex.end());

    return hex.empty() ? "0" : hex;
}

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

        BigInt(int n)
            : BigInt(DecimalToHex(to_string(n))) {}
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
            /* if (borrow != 0) {
                 cerr << "ERROR: subtraction resulted in negative number\n";
             }*/
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
        bool operator!=(const BigInt &B) const {
            if (Cmp(B) == 0) {
                return 0;
            } else {
                return 1;
            }
        }
        bool operator>=(const BigInt &B) const {
            if (*this > B && *this == B)
                return 1;
            else
                return 0;
        }

        BigInt static LongMulOneDigit(const BigInt &A, uint32_t b) {
            BigInt C;
            uint64_t carry = 0;

            for (int i = 0; i < system_size; i++) {
                uint64_t temp = (uint64_t)A.a[i] * (uint64_t)b + carry;

                C.a[i] = (uint32_t)(temp & 0xFFFFFFFFu); // нижні 32 біти
                carry = temp >> 32;                      // перенос
            }

            return C;
        }

        void LongShiftDigitsToHigh(BigInt &X, int shift) const {
            if (shift == 0)
                return;

            for (int i = system_size - 1; i >= shift; i--) {
                X.a[i] = X.a[i - shift];
            }
            for (int i = 0; i < shift; i++) {
                X.a[i] = 0;
            }
        }

        BigInt LongMul(const BigInt &A, const BigInt &B) const {
            BigInt C;

            for (int i = 0; i < system_size; i++) {
                BigInt temp = LongMulOneDigit(A, B.a[i]);

                LongShiftDigitsToHigh(temp, i); // temp << (i*32)
                C = C + temp;
            }

            return C;
        }
        BigInt operator*(const BigInt &B) const {
            BigInt C;
            C = LongMul(*this, B);
            return C;
        }
        BigInt operator*(int n) const {
            BigInt C;
            BigInt B = BigInt(n);
            C = LongMul(*this, B);
            return C;
        }

        BigInt static LongShiftBitsToHigh(const BigInt &X, int shiftBits) {
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

        BigInt static LongShiftBitsToLow(const BigInt &X, int shiftBits) {
            BigInt R;

            int wordShift = shiftBits / 32;
            int bitShift = shiftBits % 32;

            for (int i = 0; i < system_size - wordShift; i++) {
                R.a[i] = X.a[i + wordShift];
            }
            for (int i = system_size - wordShift; i < system_size; i++) {
                R.a[i] = 0;
            }

            if (bitShift > 0) {
                uint32_t carry = 0;
                for (int i = system_size - 1; i >= 0; i--) {
                    uint32_t current_word = R.a[i];
                    R.a[i] =
                        (current_word >> bitShift) | (carry << (32 - bitShift));
                    carry = current_word & ((1u << bitShift) - 1);
                }
            }

            return R;
        }

        int static BitLength(const BigInt &X) {
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

        BigInt operator/(const BigInt &B) const {
            BigInt Q, R;

            if (B == BigInt()) {
                cerr << "ERROR: Division by zero" << endl;
                return BigInt();
            }
            BigInt tool;
            tool.LongDivMod(*this, B, Q, R);

            return Q;
        }
        BigInt operator/(int n) {
            BigInt temp = BigInt(DecimalToHex(to_string(n)));
            return *this / temp;
        }

        BigInt operator%(BigInt &B) {
            BigInt Q, R;

            if (B == BigInt()) {
                cerr << "ERROR: Modulo by zero" << endl;
                return BigInt();
            }

            BigInt tool;
            tool.LongDivMod(*this, B, Q, R);

            return R;
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

        BigInt min(const BigInt &A, const BigInt &B) const {
            if (A < B) {
                return A;
            } else {
                return B;
            }
        }
        BigInt static BinaryAlg(const BigInt &Asource, const BigInt &Bsource) {
            BigInt A = Asource;
            BigInt B = Bsource;
            BigInt D = BigInt(1);
            while (A.GetBit(0) == 0 && B.GetBit(0) == 0) {
                A = A / 2;
                B = B / 2;
                D = D * 2;
            }
            while (A.GetBit(0) == 0) {
                A = A / 2;
            }
            while (B != BigInt()) {
                while (B.GetBit(0) == 0) {
                    B = B / 2;
                }
                if (A > B) {
                    BigInt temp = A;
                    A = B;
                    B = temp;
                }

                B = B - A;
            }
            D = D * A;
            return D;
        }

        BigInt static LCM(const BigInt &A, const BigInt &B) {
            if (A == BigInt() || B == BigInt()) {
                return BigInt();
            }

            BigInt gcd = BinaryAlg(A, B);

            BigInt temp = A / gcd;
            BigInt lcm = temp * B;

            return lcm;
        }

        static BigInt BarrettMu(const BigInt &n) {
            int k = BitLength(n);

            // 2^(2k)
            BigInt twoPow2k;
            twoPow2k.a[(2 * k) / WORD_BITS] = 1u << ((2 * k) % WORD_BITS);

            BigInt mu = twoPow2k / n;
            return mu;
        }

        static BigInt BarrettReduction(const BigInt &x, const BigInt &n,
                                       const BigInt &mu) {
            int k = BitLength(n);

            BigInt q1 = LongShiftBitsToLow(x, k - 1);
            BigInt q2 = q1 * mu;
            BigInt q3 = LongShiftBitsToLow(q2, k + 1);

            BigInt r = x - (q3 * n);

            while (r >= n)
                r = r - n;
            while (r < BigInt())
                r = r + n;

            return r;
        }

        BigInt ModBarrett(const BigInt &n, const BigInt &mu) const {
            return BarrettReduction(*this, n, mu);
        }

        BigInt static PowModBarrett(BigInt A, BigInt B, const BigInt &n,
                                    const BigInt &mu) {
            BigInt C(1);

            A = BarrettReduction(A, n, mu);

            for (int i = 0; i < BitLength(B); i++) {
                if (B.GetBit(i) == 1) {
                    C = BarrettReduction(C * A, n, mu);
                }
                A = BarrettReduction(A * A, n, mu);
            }
            return C;
        }
};

int main() {

    BigInt A("96f4021949887b8a63f4da2ad78c1cd023da79a3eb26870f2315dd92d817afee6"
             "a49da7f35686e3bcb4d8af86f148744d971adbf1ca01c0df2759e107e41f45d7b"
             "81d42f03ffe8b9182cddebe48f47e2376ca2ace553fc772d977bfcbeb1e205331"
             "a66cb11f344a7190858411483c4be250eb7546ace290bc25c799d24f86f3c");
    BigInt B("b28d6a12c10483fd0e5e05b2ea4c34a54a946750019a5a2f327cc19ba598bd6ea"
             "fcec153670fa75109d9efdd2363a85c1d4bd9d77a1670875d12d92c3ab8ab42b9"
             "8d174f0a3ca2dc7043d46eecb662308ae953090d545ce49945bf18d462c034884"
             "12c1c2fc360d2575d532d1dfb86706e7dd256bb131795ec01bd26fdf8b1ab");

    cout << "A = ";
    A.Print();

    cout << "B = ";
    B.Print();

    BigInt n(
        "96f4021949887b8a63f4da2ad78c1cd023da79a3eb26870f2315dd92d817afee6"
        "a49da7f35686e3bcb4d8af86f148744d971adbf1ca01c0df2759e107e41f45d7b"
        "81d42f03ffe8b9182cddebe48f47e2376ca2ace553fc772d977bfcbeb1e20533");
    cout << "n = ";
    n.Print();
    BigInt mu = BigInt::BarrettMu(n);

    cout << "mu = ";
    mu.Print();

    BigInt S = A + B;
    cout << "A + B = ";
    S.Print();
    BigInt res = S % n;
    cout << "A + B mod n = ";
    res.Print();

    BigInt D = A - B;
    cout << "A - B = ";
    D.Print();
    res = D % n;
    cout << "A - B mod n = ";
    res.Print();

    BigInt M;
    M = M.LongMul(A, B);
    cout << "A * B = ";
    M.Print();
    res = M % n;
    cout << "A * B mod n = ";
    res.Print();

    BigInt Q, R;
    Q.LongDivMod(A, B, Q, R);
    cout << "A / B = ";
    Q.Print();
    cout << "A % B = ";
    R.Print();

    /* BigInt L;
     L = L.Gorner(A, B);
     cout << "A ^ B = ";
     L.Print();
    */
    res = BigInt::PowModBarrett(A, B, n, mu);
    cout << "A ^ B mod n = ";
    res.Print();

    BigInt L1;
    L1 = L1.Gorner(A, 2);
    cout << "A ^ 2 = ";
    L1.Print();
    res = L1 % n;
    cout << "A ^ 2 mod n = ";
    res.Print();

    BigInt d = BigInt::BinaryAlg(A, B);
    cout << "GCD = ";
    d.Print();

    BigInt lcm = BigInt::LCM(A, B);

    cout << "LCM = ";
    lcm.Print();

    return 0;
}
