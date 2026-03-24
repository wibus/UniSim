#include "definitions.h"

namespace unisim
{
namespace bruneton
{

double CieColorMatchingFunctionTableValue(double wavelength, int column)
{
    if (wavelength <= kLambdaMin || wavelength >= kLambdaMax)
    {
        return 0.0;
    }

    double u = (wavelength - kLambdaMin) / 5.0;
    int row = static_cast<int>(std::floor(u));
    assert(row >= 0 && row + 1 < 95);
    assert(CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row] <= wavelength
           && CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1)] >= wavelength);
    u -= row;
    return CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row + column] * (1.0 - u)
           + CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1) + column] * u;
}

double Interpolate(const std::vector<double> &wavelengths,
                   const std::vector<double> &wavelength_function,
                   double wavelength)
{
    assert(wavelength_function.size() == wavelengths.size());
    if (wavelength < wavelengths[0])
    {
        return wavelength_function[0];
    }

    for (unsigned int i = 0; i < wavelengths.size() - 1; ++i)
    {
        if (wavelength < wavelengths[i + 1])
        {
            double u = (wavelength - wavelengths[i]) / (wavelengths[i + 1] - wavelengths[i]);
            return wavelength_function[i] * (1.0 - u) + wavelength_function[i + 1] * u;
        }
    }
    return wavelength_function[wavelength_function.size() - 1];
}

void ConvertSpectrumToLinearSrgb(const std::vector<double> &wavelengths,
                                 const std::vector<double> &spectrum,
                                 double *r,
                                 double *g,
                                 double *b)
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    const int dlambda = 1;
    for (int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda)
    {
        double value = Interpolate(wavelengths, spectrum, lambda);
        x += CieColorMatchingFunctionTableValue(lambda, 1) * value;
        y += CieColorMatchingFunctionTableValue(lambda, 2) * value;
        z += CieColorMatchingFunctionTableValue(lambda, 3) * value;
    }
    *r = MAX_LUMINOUS_EFFICACY * (XYZ_TO_SRGB[0] * x + XYZ_TO_SRGB[1] * y + XYZ_TO_SRGB[2] * z)
         * dlambda;
    *g = MAX_LUMINOUS_EFFICACY * (XYZ_TO_SRGB[3] * x + XYZ_TO_SRGB[4] * y + XYZ_TO_SRGB[5] * z)
         * dlambda;
    *b = MAX_LUMINOUS_EFFICACY * (XYZ_TO_SRGB[6] * x + XYZ_TO_SRGB[7] * y + XYZ_TO_SRGB[8] * z)
         * dlambda;
}

} // namespace bruneton
} // namespace unisim
