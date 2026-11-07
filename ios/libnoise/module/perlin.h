// Minimal libnoise-compatible Perlin module used for the iOS build.
// Only the subset of the libnoise API used by eZeus is provided.
#ifndef IOS_LIBNOISE_PERLIN_H
#define IOS_LIBNOISE_PERLIN_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <random>

namespace noise {
namespace module {

class Perlin {
public:
    Perlin() { SetSeed(0); }

    void SetSeed(const int seed) {
        mSeed = seed;
        std::iota(mPerm, mPerm + 256, 0);
        std::mt19937 gen(static_cast<std::uint32_t>(seed));
        std::shuffle(mPerm, mPerm + 256, gen);
        for(int i = 0; i < 256; i++) mPerm[256 + i] = mPerm[i];
    }
    int GetSeed() const { return mSeed; }

    void SetOctaveCount(const int c) { mOctaves = std::max(1, std::min(30, c)); }
    void SetFrequency(const double f) { mFrequency = f; }
    void SetPersistence(const double p) { mPersistence = p; }
    void SetLacunarity(const double l) { mLacunarity = l; }

    double GetValue(double x, double y, double z) const {
        double value = 0.0;
        double amp = 1.0;
        x *= mFrequency;
        y *= mFrequency;
        z *= mFrequency;
        for(int i = 0; i < mOctaves; i++) {
            value += amp * gradientNoise(x, y, z);
            x *= mLacunarity;
            y *= mLacunarity;
            z *= mLacunarity;
            amp *= mPersistence;
        }
        return value;
    }

private:
    static double fade(const double t) {
        return t*t*t*(t*(t*6. - 15.) + 10.);
    }
    static double lerp(const double t, const double a, const double b) {
        return a + t*(b - a);
    }
    static double grad(const int hash, const double x,
                       const double y, const double z) {
        const int h = hash & 15;
        const double u = h < 8 ? x : y;
        const double v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    double gradientNoise(const double x, const double y, const double z) const {
        const int X = static_cast<int>(std::floor(x)) & 255;
        const int Y = static_cast<int>(std::floor(y)) & 255;
        const int Z = static_cast<int>(std::floor(z)) & 255;
        const double xf = x - std::floor(x);
        const double yf = y - std::floor(y);
        const double zf = z - std::floor(z);
        const double u = fade(xf);
        const double v = fade(yf);
        const double w = fade(zf);
        const int A = mPerm[X] + Y;
        const int AA = mPerm[A] + Z;
        const int AB = mPerm[A + 1] + Z;
        const int B = mPerm[X + 1] + Y;
        const int BA = mPerm[B] + Z;
        const int BB = mPerm[B + 1] + Z;
        return lerp(w,
                    lerp(v,
                         lerp(u, grad(mPerm[AA], xf, yf, zf),
                                 grad(mPerm[BA], xf - 1, yf, zf)),
                         lerp(u, grad(mPerm[AB], xf, yf - 1, zf),
                                 grad(mPerm[BB], xf - 1, yf - 1, zf))),
                    lerp(v,
                         lerp(u, grad(mPerm[AA + 1], xf, yf, zf - 1),
                                 grad(mPerm[BA + 1], xf - 1, yf, zf - 1)),
                         lerp(u, grad(mPerm[AB + 1], xf, yf - 1, zf - 1),
                                 grad(mPerm[BB + 1], xf - 1, yf - 1, zf - 1))));
    }

    int mSeed = 0;
    int mOctaves = 6;
    double mFrequency = 1.0;
    double mPersistence = 0.5;
    double mLacunarity = 2.0;
    int mPerm[512];
};

} // namespace module
} // namespace noise

#endif
