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

// The returned constants are in lumen.nm / watt.
void ComputeSpectralRadianceToLuminanceFactors(const std::vector<double> &wavelengths,
                                               const std::vector<double> &solar_irradiance,
                                               double lambda_power,
                                               double *k_r,
                                               double *k_g,
                                               double *k_b)
{
    *k_r = 0.0;
    *k_g = 0.0;
    *k_b = 0.0;
    double solar_r = Interpolate(wavelengths, solar_irradiance, kLambdaR);
    double solar_g = Interpolate(wavelengths, solar_irradiance, kLambdaG);
    double solar_b = Interpolate(wavelengths, solar_irradiance, kLambdaB);
    int dlambda = 1;
    for (int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda)
    {
        double x_bar = CieColorMatchingFunctionTableValue(lambda, 1);
        double y_bar = CieColorMatchingFunctionTableValue(lambda, 2);
        double z_bar = CieColorMatchingFunctionTableValue(lambda, 3);
        const double *xyz2srgb = XYZ_TO_SRGB;
        double r_bar = xyz2srgb[0] * x_bar + xyz2srgb[1] * y_bar + xyz2srgb[2] * z_bar;
        double g_bar = xyz2srgb[3] * x_bar + xyz2srgb[4] * y_bar + xyz2srgb[5] * z_bar;
        double b_bar = xyz2srgb[6] * x_bar + xyz2srgb[7] * y_bar + xyz2srgb[8] * z_bar;
        double irradiance = Interpolate(wavelengths, solar_irradiance, lambda);
        *k_r += r_bar * irradiance / solar_r * pow(lambda / kLambdaR, lambda_power);
        *k_g += g_bar * irradiance / solar_g * pow(lambda / kLambdaG, lambda_power);
        *k_b += b_bar * irradiance / solar_b * pow(lambda / kLambdaB, lambda_power);
    }
    *k_r *= MAX_LUMINOUS_EFFICACY * dlambda;
    *k_g *= MAX_LUMINOUS_EFFICACY * dlambda;
    *k_b *= MAX_LUMINOUS_EFFICACY * dlambda;
}

} // namespace bruneton
} // namespace unisim
