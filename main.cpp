// Project Simulation code
// Copyright (C) zyy_bug 2026.

/*
	This code is integrated as a library-code union.
	This code is inpremented in the hope that it will be useful, and
	No warranty is given.
	If you see this text, this program only supports Windows platform.

	Technical details:

	I.How to compile
		1. Install normal Dev-C++ at here: https://sourceforge.net/projects/orwelldevcpp/files/latest/download
		2. Install latest TDM-GCC compiler (this build uses 10.3.0): https://jmeubank.github.io/tdm-gcc/
		3. Replace MinGW64 compiler with the one you've just installed.
		4. Make sure your Dev-C++ is named as "Dev-Cpp" and is in same directory
		   with "Project Simulation", like this:
		   -/Dev-C++
		    /Project_Simulation
		5. Open "Project Simulation.dev", check requirements and dependencies, and run.

	II.Requirements to check before run
		Though already set, there can be things lost or forgotten.
		This is a checklist for a runable executable.

		I.Folder files
			imconfig.h
			imgui.cpp
			imgui.h
			imgui_draw.cpp
			imgui_impl_dx11.cpp
			imgui_impl_dx11.h
			imgui_impl_win32.cpp
			imgui_impl_win32.h
			imgui_internal.h
			imgui_tables.cpp
			imgui_widgets.cpp
			imstb_rectpack.h
			imstb_textedit.h
			imstb_truetype.h
			main.cpp
			Makefile.win
			Project Simulation.dev
			The Beginning.mp3
			Les Route.mp3
			Stars.mp3

		II.Project files
			main.cpp

		III.C++ command
		-std=c++20 -limgui -limgui_impl_win32 -limgui_impl_dx11 -ld3d11 -ldxgi -ld3dcompiler -ldwmapi -luser32 -lgdi32 -limm32 -lwinmm


		IV.Linker
imgui.o
imgui_draw.o
imgui_impl_dx11.o
imgui_impl_win32.o
imgui_tables.o
imgui_widgets.o

		V.Binaries
		../Dev-Cpp/MinGW64/bin
		../Dev-Cpp/MinGW64/gdb64/bin (Optional)

		Other settings are mostly default.

	// Code Division -------------------------
	    Section 1 - Necessary header files and initialization.
	    Section 2 - Real class and helper initialization.
	    Section 3 - Time related items initialization.
	    Section 4 - Player and major functions initialization.
        Section 5 - Audio initialization.
        Section 6 - GUI related items initialization.
        Section 7 - Game-loop function.
        Section 8 - Save-Read Management System.
*/

// ============================================================
// Section 1 - Necessary header files and initialization.
// ============================================================

//Necessary header files.
#include <bits/stdc++.h>
#include <compare>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <unordered_map>
#include <cctype>
#include <stdexcept>
#include <cstdlib>

//Enable portable code after GUI.
//Currently only Windows avaliable.
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>

//GUI-related headers.
//Here, we use utility ImGui, which also requires DXD11 installed.
#include <dwmapi.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "dwmapi.lib")
#include <io.h>

static ID3D11Device*           g_pd3dDevice = nullptr;
static ID3D11DeviceContext*    g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*         g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static UINT                    g_ResizeWidth = 0, g_ResizeHeight = 0;
static bool                    g_gui_running = false;
static bool                    g_gui_active = false;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

//Placeholder.
#elif __ANDROID__
#include <unistd.h>
#else

#endif


//Whether to use GUI or not.
//It may be strange to see false here, but it should be,
//for non-Windows platforms.
bool g_use_gui = false;


using namespace std;

// ============================================================
// Section 2 - Number class and helper initialization.
// ============================================================

static const long double LD_PI   = 3.14159265358979323846L;
static const long double LD_PI_2 = 1.57079632679489661923L;
static const long double LD_E    = 2.7182818284590452354L;
static const long double LD_LN10 = 2.30258509299404568402L;
static const long double LD_LOG10E = 0.43429448190325182765L;

//to_string_prec(a, int prec)
//Input:
//    a           An arbitrary number. (Less than the limit of given limit, see std::isfinite().)
//    prec        Digits to preserve.
//Returns a string as the number with the given precision.
//to_string_prec(1.2345, 3) = "1.235".
// ----------------------------------------------------------------------
template <typename num>
string to_string_prec(num a, int prec) {
	ostringstream oss;
	long double v = static_cast<long double>(a);
	if (!std::isfinite(v)) {
		return std::isnan(v) ? "NaN" : (v > 0 ? "inf" : "-inf");
	}

	long double av = abs(v);

	// In cases where, digits are too much to output, we use scientific output.
	if (av != 0 && (av >= 1e10L || av < 1e-10L)) {
		oss << std::scientific << std::setprecision(prec) << v;
	}
	// Otherwise.
	else {
		oss << std::fixed << std::setprecision(prec) << v;
	}
	return oss.str();
}

static FILE* g_log = nullptr;
static void log_init() {
	g_log = fopen("debug.log", "w");
}
static void log_line(const std::string& s) {
	if (!g_log) log_init();
	if (g_log) {
		fputs(s.c_str(), g_log);
		fputc('\n', g_log);
		fflush(g_log);
	}
	std::cout << s << std::endl;
}

// ----------------------------------------------------------------------
// Real
// Real allows a much greater number to be stored.
// layer = 0: x = val.
// layer = 1: x = 10^(val)
// layer = 2: x = 10^(10^val), etc.
// sign = false: 1/x.
// neg = -1: negative number.
// Construction: x.
// ----------------------------------------------------------------------
class Real {
	public:
		int         neg   = 1;
		bool        sign  = true;
		uint64_t    layer = 0;
		long double val   = 0.0L;

		// ---------- Construction ----------
		constexpr Real() : neg(1), sign(true), layer(0), val(0.0L) {}

		Real(long double v) : neg(1), sign(true), layer(0), val(v) {
			normalize_inplace();
		}
		Real(long double v, long double l)
			: neg(1), sign(true), layer((uint64_t)l), val(v) {
			normalize_inplace();
		}
		Real(bool s, long double v, long double l)
			: neg(1), sign(s), layer((uint64_t)l), val(v) {
			normalize_inplace();
		}
		Real(bool s, int n, long double v, long double l)
			: neg(n < 0 ? -1 : 1), sign(s), layer((uint64_t)l), val(v) {
			normalize_inplace();
		}
		Real(int n, long double v, long double l)
			: neg(n < 0 ? -1 : 1), sign(true), layer((uint64_t)l), val(v) {
			normalize_inplace();
		}

		// ---------- Statements ----------
		bool is_zero() const {
			return sign && layer == 0 && val == 0.0L;
		}
		bool is_one() const {
			return neg > 0 && sign && layer == 0 && val == 1.0L;
		}
		bool is_negative() const {
			return neg < 0 && !is_zero();
		}
		bool is_positive() const {
			return neg > 0 && !is_zero();
		}

		[[nodiscard]] Real normalize() const {
			Real n = *this;
			n.normalize_inplace();
			return n;
		}

		// ---------- normalize ----------
		void normalize_inplace();

		// ---------- Comparison ----------
		static int cmp_abs(const Real& a, const Real& b);
		std::partial_ordering operator<=>(const Real& other) const;
		bool operator==(const Real& other) const;

		// ---------- Operators ----------
		Real operator+(const Real& other) const;
		Real operator-(const Real& other) const;
		Real operator*(const Real& other) const;
		Real operator/(const Real& other) const;

		Real& operator+=(const Real& other) {
			*this = *this + other;
			return *this;
		}
		Real& operator-=(const Real& other) {
			*this = *this - other;
			return *this;
		}
		Real& operator*=(const Real& other) {
			*this = *this * other;
			return *this;
		}
		Real& operator/=(const Real& other) {
			*this = *this / other;
			return *this;
		}

		// ---------- Helper ----------
		static Real invert(const Real& x) {
			Real r = x.normalize();
			r.sign = !r.sign;
			return r;
		}
		static Real log10_of(const Real& x);
		static Real pow10_of(const Real& x);

		[[nodiscard]] Real pow(const Real& exp) const;
		[[nodiscard]] Real log(const Real& other) const;
		[[nodiscard]] Real pow_sum(const Real& other) const;

		[[nodiscard]] Real dilate(const Real& exp) const;
		[[nodiscard]] Real tetrate(const Real& exp) const;
		[[nodiscard]] Real tetrate(const Real& exp, const Real& layer) const;

		[[nodiscard]] Real abs() const {
			return Real(1, this->val, this->layer);
		}

		// ---------- Transform ----------
		long double _to_long_double() const;
		[[nodiscard]] std::string toString() const;

		friend std::ostream& operator<<(std::ostream& os, const Real& n) {
			os << n.toString();
			return os;
		}
};

const Real ZERO = Real(0.0L);
const Real ONE = Real(1.0L);
const Real TWO = Real(2.0L);
const Real FIVE = Real(5.0L);
const Real TEN = Real(10.0L);

// Helper function for normalize().
void Real::normalize_inplace() {
	if (std::isnan(val)) {
		*this = Real();
		return;
	}
	if (std::isinf(val)) {
		layer = UINT64_MAX;
		val = 10.0L;
		return;
	}

	if (val == 0.0L) {
		neg = 1;
		sign = true;
		layer = 0;
		return;
	}
	if (val < 0.0L) {
		neg = -neg;
		val = -val;
	}

	while (val < 1.0L) {
		if (layer == 0) {
			sign = !sign;
			val  = 1.0L / val;
			break;
		} else {
			val = powl(10.0L, val);
			layer--;
		}
	}
	while (val >= 10.0L) {
		val = log10l(val);
		layer++;
	}
}

class Number;

template<typename T>
Real _to_Real(const T& x) {
	using U = std::decay_t<T>;

	if constexpr (std::is_same_v<U, Real>) {
		return x;
	} else if constexpr (std::is_same_v<U, Number>) {
		return x.r;
	} else if constexpr (std::is_arithmetic_v<U>) {
		long double v = static_cast<long double>(x);
		if (v == 0.0L || std::isnan(v)) return Real();
		bool neg = v < 0.0L;
		if (neg) v = -v;
		if (std::isinf(v)) {
			Real r;
			r.neg   = neg ? -1 : 1;
			r.layer = UINT64_MAX;
			r.val   = 10.0L;
			return r;
		}
		return Real(true, neg ? -1 : 1, v, 0.0L).normalize();
	} else {
		return Real();
	}
}


// Returns e^x.
static Real real_exp(const Real& x) {
	return TEN.pow(x * _to_Real(LD_LOG10E));
}

// Comparing absolute value.
int Real::cmp_abs(const Real& a, const Real& b) {
	if (a.sign != b.sign) return a.sign ? 1 : -1;
	int c;
	if      (a.layer != b.layer) c = (a.layer > b.layer) ?  1 : -1;
	else if (a.val   != b.val)   c = (a.val   > b.val)   ?  1 : -1;
	else                         c = 0;
	return a.sign ? c : -c;
}

std::partial_ordering Real::operator<=>(const Real& other) const {
	const Real& a = *this;
	const Real& b = other;

	if (a.is_zero() && b.is_zero()) return std::partial_ordering::equivalent;
	if (a.is_zero())
		return b.neg > 0 ? std::partial_ordering::less
		       : std::partial_ordering::greater;
	if (b.is_zero())
		return a.neg > 0 ? std::partial_ordering::greater
		       : std::partial_ordering::less;
	if (a.neg != b.neg)
		return a.neg > 0 ? std::partial_ordering::greater
		       : std::partial_ordering::less;

	int c = cmp_abs(a, b);
	if (a.neg < 0) c = -c;
	if (c > 0) return std::partial_ordering::greater;
	if (c < 0) return std::partial_ordering::less;
	return std::partial_ordering::equivalent;
}

bool Real::operator==(const Real& other) const {
	Real a = normalize(), b = other.normalize();
	if (a.is_zero() && b.is_zero()) return true;
	return a.neg == b.neg && a.sign == b.sign
	       && a.layer == b.layer && a.val == b.val;
}

static Real add_pos_ge1(const Real& a, const Real& b) {
	if (a.layer == 0 && b.layer == 0) {
		Real r;
		r.val = a.val + b.val;
		r.normalize_inplace();
		return r;
	}
	auto to_exp = [](const Real& x) -> long double {
		if (x.layer == 1) {
			long double v = x.val;
			return x.sign ? v : -v;
		}
		long double v = log10l(x.val);
		return x.sign ? v : -v;
	};
	if ((a.layer <= 1) && (b.layer <= 1)) {
		long double ea = to_exp(a);
		long double eb = to_exp(b);
		long double m  = std::max(ea, eb);
		long double d  = std::fabs(ea - eb);
		if (d > 20.0L) return (ea > eb) ? a : b;
		long double nl = m + log10l(1.0L + powl(10.0L, -d));
		Real r;
		r.layer = 1;
		r.val = nl;
		r.normalize_inplace();
		return r;
	}

	Real La = Real::log10_of(a);
	Real Lb = Real::log10_of(b);
	bool a_ge_b = (La >= Lb);
	Real L_max = a_ge_b ? La : Lb;
	Real L_min = a_ge_b ? Lb : La;
	Real L_diff = L_max - L_min;
	if (L_diff >= _to_Real(18)) return a_ge_b ? a : b;
	if (L_max.layer >= 2)         return a_ge_b ? a : b;

	Real t = Real::pow10_of(ZERO - L_diff);
	Real correction = Real::log10_of(t + ONE);
	return Real::pow10_of(L_max + correction);
}

static long double to_ld(const Real& x) {
	Real x0 = x.normalize();
	if (x0.layer == 0) {
		long double v = x0.val;
		if(x0.neg == -1) v = -v;
		return x0.sign ? v : 1.0L / v;
	}
	if (x0.layer == 1) {
		long double v = powl(10.0L, x0.val);
		if(x0.neg == -1) v = 1L / v;
		return x0.sign ? v : 1.0L / v;
	}
	if (x0.layer == 2 && x0.val < 3.1L) {
		long double inner = powl(10.0L, x0.val);   // 10^val
		long double outer = powl(10.0L, inner);    // 10^(10^val)
		if (!x0.sign) outer = 1.0L / outer;
		if (x0.neg < 0) outer = -outer;
		return outer;
	}
	return x.sign ? 1e300L : 0.0L;
}

static Real add_same_sign(const Real& a, const Real& b) {
	Real aa = a, bb = b;
	aa.neg = 1;
	bb.neg = 1;
	int sgn = a.neg;

	if (aa.sign && bb.sign) {
		Real r = add_pos_ge1(aa, bb);
		r.neg = sgn;
		return r;
	}
	if (!aa.sign && !bb.sign) {
		long double va = to_ld(aa);
		long double vb = to_ld(bb);
		Real r;
		r.val = va + vb;
		r.normalize_inplace();
		r.neg = sgn;
		return r;
	}

	long double va = to_ld(aa);
	long double vb = to_ld(bb);
	if (va < vb) std::swap(va, vb);
	if (vb < va * 1e-18L) {
		Real r = (va == to_ld(aa)) ? aa : bb;
		r.neg = sgn;
		return r;
	}
	Real r;
	r.val = va + vb;
	r.normalize_inplace();
	r.neg = sgn;
	return r;
}

static Real sub_same_sign(const Real& a, const Real& b) {
	Real aa = a, bb = b;
	aa.neg = 1;
	bb.neg = 1;
	int sgn = a.neg;

	long double va = to_ld(aa);
	long double vb = to_ld(bb);
	if (vb == 0.0L) {
		Real r = aa;
		r.neg = sgn;
		return r;
	}

	if (vb < va * 1e-18L) {
		Real r = aa;
		r.neg = sgn;
		return r;
	}

	Real r;
	r.val = va - vb;
	r.normalize_inplace();
	r.neg = sgn;
	return r;
}

Real Real::operator+(const Real& other) const {
	Real a = normalize(), b = other.normalize();
	if (a.is_zero()) return b;
	if (b.is_zero()) return a;

	if (a.neg == b.neg) {
		return add_same_sign(a, b);
	}
	int c = cmp_abs(a, b);
	if (c == 0) return Real();
	if (c > 0) {
		Real r = sub_same_sign(a, b);
		r.neg = a.neg;
		return r;
	} else {
		Real r = sub_same_sign(b, a);
		r.neg = b.neg;
		return r;
	}
}

Real Real::operator-(const Real& other) const {
	Real b = other.normalize();
	b.neg = -b.neg;
	return (*this) + b;
}

// log10(x) : Reduce layer by 1, log10 if layer = 0.
Real Real::log10_of(const Real& x) {
	Real r = x.normalize();
	if (r.is_zero() || r.neg < 0) return Real();

	Real L;
	if (r.layer > 0) {
		// log10(T(L, V)) = T(L-1, V)
		L.layer = r.layer - 1;
		L.val   = r.val;
	} else {
		// layer == 0:log10(val)
		long double lv = log10l(r.val);
		L.val = lv;
	}
	L.normalize_inplace();
	if (!r.sign) L.neg = -L.neg;   // x = 1/T(L,V) ? log10 = -T(L-1,V)
	return L;
}

Real Real::pow10_of(const Real& x) {
	Real y = x.normalize();
	if (y.is_zero()) return Real(1.0L);

	if (y.layer >= 2) {
		if (!y.sign) return Real(1.0L);
		Real out;
		out.neg = 1;
		out.sign = (y.neg > 0);
		out.layer = y.layer + 1;
		out.val = y.val;
		return out.normalize();
	}

	long double y_ld;
	if (y.layer == 0) {
		y_ld = y.val;
		if (!y.sign) y_ld = 1.0L / y_ld;
		if (y.neg < 0) y_ld = -y_ld;
	} else { // y.layer == 1
		if (y.sign) {
			y_ld = powl(10.0L, y.val);
			if (y.neg < 0) y_ld = -y_ld;
			if (!std::isfinite(y_ld)) {
				Real out;
				out.neg = 1;
				out.sign = true;
				out.layer = 2;
				out.val = y.val;
				return out.normalize();
			}
		} else {
			y_ld = powl(10.0L, -y.val);
			if (y.neg < 0) y_ld = -y_ld;
		}
	}

	long double r = powl(10.0L, y_ld);
	if (!std::isfinite(r)) {
		Real out;
		out.neg = 1;
		out.sign = true;
		out.layer = 1;
		out.val = y_ld;
		out.normalize_inplace();
		return out;
	}
	Real out;
	out.val = r;
	out.normalize_inplace();
	return out;
}

Real Real::operator*(const Real& other) const {
	Real a = normalize(), b = other.normalize();
	if (a.is_zero() || b.is_zero()) return Real();

	int rn = a.neg * b.neg;

	a.neg = 1;
	b.neg = 1;

	Real la = log10_of(a);
	Real lb = log10_of(b);
	Real s  = la + lb;
	Real r  = pow10_of(s);

	r.neg = rn;
	return r.normalize();
}

Real Real::operator/(const Real& other) const {
	Real b = other.normalize();
	if (b.is_zero()) return Real();
	return (*this) * invert(b);
}

Real Real::pow(const Real& exponent) const {
	Real b = normalize();
	Real e = exponent.normalize();
	if (b.is_zero()) return Real();
	if (e.is_zero()) return Real(1.0L);
	if (b.neg < 0)   return Real();

	// x^y = 10^(log10(x) * y)
	Real lb = log10_of(b);
	Real le = lb * e;
	return pow10_of(le);
}

Real Real::log(const Real& base) const {
	Real a = normalize(), b = base.normalize();
	if (a.is_zero() || b.is_one() || b.is_zero()) return Real();
	if (a.neg < 0 || b.neg < 0) return Real();
	return log10_of(a) / log10_of(b);
}

Real Real::pow_sum(const Real& other) const {
	Real nn = this->normalize();
	Real ss = other.normalize();
	if (nn.neg < 0 || ss.is_zero()) return Real(1.0L);
	if (nn.is_zero() || nn.is_one()) return Real(1.0L);

	Real shifted  = nn + Real(0.5L);
	Real exp_one  = ss + Real(1.0L);
	if (exp_one.is_zero()) return shifted.log(Real(10.0L));

	Real result = shifted.pow(exp_one) / exp_one;
	if (result.neg < 0) result = Real();
	return result.normalize();
}

// Dilation: a^^b = 10^(log10(a)^b)
Real Real::dilate(const Real& exp) const {
	Real n = this->normalize();
	if(n <= TEN) return n;
	return TEN.pow(this->log(TEN).pow(exp));
}

// Tetration: a^^^b = 10^(10^(log10(log10(a))^b))
Real Real::tetrate(const Real& exp) const {
	Real n = this->normalize();
	if(n <= TEN.pow(TEN)) return n;
	return TEN.pow(TEN.pow(this->log(TEN).log(TEN).pow(exp)));
}

// Tetrate: Dilation = 1, Tetration = 2, etc.
Real Real::tetrate(const Real& exp, const Real& layer) const {
	Real aa = this->normalize();
	Real a = this->normalize();
	for(Real i = ONE; i <= layer; i += ONE) {
		a = a.log(TEN);
		if(a <= ZERO) return aa;
	}
	a = a.pow(exp);
	for(Real i = ONE; i <= layer; i += ONE) a = TEN.pow(a);
	return a;
}

long double Real::_to_long_double() const {
	Real a = normalize();
	if (a.is_zero()) return 0.0L;

	long double v;
	if (a.layer == 0) {
		v = a.val;
		if (!a.sign) v = 1.0L / v;
	} else if (a.layer == 1) {
		v = powl(10.0L, a.val);
		if (!std::isfinite(v)) return (a.neg > 0) ? INFINITY : -INFINITY;
		if (!a.sign) v = 1.0L / v;
	} else {
		return (a.neg > 0) ? INFINITY : -INFINITY;
	}
	return (a.neg > 0) ? v : -v;
}

static std::string add_commas(const std::string& intpart) {
	std::string sign;
	std::string s = intpart;
	if (!s.empty() && s[0] == '-') {
		sign = "-";
		s = s.substr(1);
	}
	std::string result;
	int n = (int)s.size();
	for (int i = 0; i < n; ++i) {
		if (i > 0 && (n - i) % 3 == 0) result += ',';
		result += s[i];
	}
	return sign + result;
}

// 1234.5678 -> "1,234.567800"
static std::string format_with_commas(long double v, int prec) {
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(prec) << v;
	std::string s = oss.str();
	size_t dot = s.find('.');
	if (dot == std::string::npos) return add_commas(s);
	return add_commas(s.substr(0, dot)) + s.substr(dot);
}

// Helper to format a long double as an integer string (rounded)
static string fmt_int(long double v) {
	// Round to nearest integer
	long double rounded = roundl(v);
	ostringstream oss;
	// Use fixed precision 0 to ensure no decimal point
	oss << std::fixed << std::setprecision(0) << rounded;
	return oss.str();
}

// Helper to format mantissa with 3 decimal places
static string fmt_mantissa(long double v) {
	ostringstream oss;
	oss << std::fixed << std::setprecision(3) << v;
	return oss.str();
}

std::string Real::toString() const {
	Real a = normalize();
	if (a.is_zero()) return "0.000000";

	std::ostringstream oss;

	if (a.layer <= 1) {
		long double v;
		if (a.layer == 0) v = a.val;
		else              v = powl(10.0L, a.val);
		if (!a.sign) v = 1.0L / v;
		if (a.neg < 0) v = -v;

		long double av = fabsl(v);
		if (av >= 1e-3L && av < 1e10L) {
			return format_with_commas(v, 6);
		} else {
			long double e = floorl(log10l(av));
			long double m = av / powl(10.0L, e);
			if (m >= 9.99L) {
				m /= 10.0L;
				e += 1.0L;
			}
			if (m <  1.0L)    {
				m *= 10.0L;
				e -= 1.0L;
			}
			if (m >= 9.9995L) {  // rounds up to "10.000" at 3 dp -> renormalise mantissa
				m = 1.0L;
				e += 1.0L;
			}
			std::string s = fmt_mantissa(m) + "e" + fmt_int(e);
			if (v < 0) s = "-" + s;
			return s;
		}
	}

	if (a.val < 15.0L && a.layer <= 64) {
		long double exp0 = powl(10.0L, a.val);
		long double e    = floorl(exp0);
		long double m    = powl(10.0L, exp0 - e);
		if (m >= 9.99L) {
			m /= 10.0L;
			e += 1.0L;
		}
		if (m <  1.0L)  {
			m *= 10.0L;
			e -= 1.0L;
		}
		if (m >= 9.9995L) {  // rounds up to "10.000" at 3 dp -> renormalise mantissa
			m = 1.0L;
			e += 1.0L;
		}
		if (!a.sign) oss << "1/";
		for (uint64_t i = 0; i + 2 < a.layer; ++i) oss << "e";
		oss << fmt_mantissa(m) << "e" << fmt_int(e);
	} else if (a.layer <= 8) {
		if (!a.sign) oss << "1/";
		for (uint64_t i = 0; i < a.layer - 1; ++i) oss << "e";
		oss << "e" << fmt_mantissa(a.val);
	} else {
		if (!a.sign) oss << "1/";
		oss << "N(" << fmt_mantissa(a.val) << "," << a.layer << ")";
	}

	std::string s = oss.str();
	if (a.neg < 0) s = "-" + s;
	return s;
}

const Real inf = Real(2).pow(_to_Real(1024));

Real floor(const Real& a) {
	Real x = a.normalize();

	if (x.is_zero()) return Real();

	if (x.neg < 0) {
		Real pos = x;
		pos.neg = 1;

		Real f = floor(pos);

		if (f == pos) {
			Real r = f;
			r.neg = -1;
			return r;
		}
		Real r = f + Real(1.0L);
		r.neg = -1;
		return r;
	}

	// (0, 1) -> floor = 0
	if (!x.sign) return Real();

	// layer = 0
	if (x.layer == 0) {
		if (x.val < 1e15L) {
			return Real(floorl(x.val));
		}
		return x;
	}

	// layer = 1
	if (x.layer == 1 && x.val < 15.0L) {
		long double v = powl(10.0L, x.val);
		if (std::isfinite(v) && v < 1e15L) {
			return Real(floorl(v));
		}
	}

	// layer >= 2
	return x;
}

// The Number no longer stores Reality.
// It stores Ephemerality.

// ============================================================
// Complex Number
// z = r + i * im  (r = real part, im = imaginary part)
// Each part is a Real (the previous "Number" class).
// ============================================================
class Number {
	public:
		Real r;   // Real part
		Real i;   // Imaginary part

		// ---------- Construction ----------
		Number() : r(ZERO), i(ZERO) {}
		Number(const Real& re)               : r(re),     i(ZERO) {}
		Number(const Real& re, const Real& im) : r(re),   i(im)    {}
		Number(long double re)               : r(re),     i(ZERO) {}
		Number(long double re, long double im): r(re),    i(im)    {}

		Number(const Number& other) = default;

		// ---------- Predicates ----------
		bool is_zero()    const {
			return r.is_zero() && i.is_zero();
		}
		bool is_real()    const {
			return i.is_zero();
		}
		bool is_imag()    const {
			return r.is_zero() && !i.is_zero();
		}
		bool is_one()     const {
			return r.is_one()  && i.is_zero();
		}
		[[nodiscard]] bool is_negative() const;
		[[nodiscard]] bool is_positive() const;

		// ---------- Helpers ----------
		[[nodiscard]] Number conj() const {
			return Number(r, ZERO - i);
		}
		[[nodiscard]] Real  abs_sq() const {
			return r*r + i*i;
		}

		[[nodiscard]] Real abs() const {
			if (i.is_zero()) {
				Real a = r;
				if (a.is_negative()) a.neg = 1;
				return a;
			}
			return abs_sq().pow(_to_Real(0.5L));
		}

		[[nodiscard]] Real arg() const;   // in (-pi, pi]

		[[nodiscard]] Number normalize() const;
		void                normalize_inplace();

		// ---------- Arithmetic ----------
		Number operator+(const Number& o) const {
			return Number(r + o.r, i + o.i);
		}
		Number operator-(const Number& o) const {
			return Number(r - o.r, i - o.i);
		}
		Number operator-()                const {
			return Number(ZERO - r, ZERO - i);
		}

		Number operator*(const Number& o) const {
			// (a+bi)(c+di) = (ac-bd) + (ad+bc)i
			return Number(r * o.r - i * o.i, r * o.i + i * o.r);
		}

		Number operator/(const Number& o) const {
			Real d = o.abs_sq();
			if (d.is_zero()) return Number();            // division by 0 �� 0
			// (a+bi)/(c+di) = [(ac+bd) + (bc-ad)i] / (c2+d2)
			return Number((r * o.r + i * o.i) / d,
			              (i * o.r - r * o.i) / d);
		}

		Number& operator+=(const Number& o) {
			*this = *this + o;
			return *this;
		}
		Number& operator-=(const Number& o) {
			*this = *this - o;
			return *this;
		}
		Number& operator*=(const Number& o) {
			*this = *this * o;
			return *this;
		}
		Number& operator/=(const Number& o) {
			*this = *this / o;
			return *this;
		}

		Number& operator+=(const Real& o) {
			return *this += Number(o);
		}
		Number& operator-=(const Real& o) {
			return *this -= Number(o);
		}
		Number& operator*=(const Real& o) {
			return *this *= Number(o);
		}
		Number& operator/=(const Real& o) {
			return *this /= Number(o);
		}

		template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
		Number& operator+=(T b) {
			return *this += Number(_to_Real(b));
		}

		template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
		Number& operator-=(T b) {
			return *this -= Number(_to_Real(b));
		}

		template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
		Number& operator*=(T b) {
			return *this *= Number(_to_Real(b));
		}

		template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
		Number& operator/=(T b) {
			return *this /= Number(_to_Real(b));
		}

		bool operator==(const Number& o) const {
			return r == o.r && i == o.i;
		}
		bool operator!=(const Number& o) const {
			return !(*this == o);
		}

		std::partial_ordering operator<=>(const Number& o) const {
			if (i.is_zero() && o.i.is_zero()) {
				return r <=> o.r;                       // Both R.
			}
			Real ma = abs();
			Real mb = o.abs();
			auto c = ma <=> mb;
			if (c != 0) return c;                       // |a| != |b|
			auto cr = r <=> o.r;
			if (cr != 0) return cr;                     // a.r vs. b.r
			return i <=> o.i;                           // a.i vs. b.i
		}

		[[nodiscard]] long double _to_long_double() const;

		// ---------- Exponential / Logarithm / Power ----------
		[[nodiscard]] Number exp_of() const;                        // e^z
		[[nodiscard]] Number ln_of()  const;                        // ln(z)
		[[nodiscard]] Number pow(const Number& exp) const;          // z^w
		[[nodiscard]] Number log(const Number& base) const;         // log_base(z)
		[[nodiscard]] Number dilate(const Number& exp) const;
		[[nodiscard]] Number tetrate(const Number& exp) const;
		[[nodiscard]] Number tetrate(const Number& exp, const Number& layer) const;

		//Number pow(const Real& exp)  const { return pow(Number(exp)); }
		//Number log(const Real& base) const { return log(Number(base)); }

		Number dilate(const Real& exp) const {
			return dilate(Number(exp));
		}
		Number tetrate(const Real& exp) const {
			return tetrate(Number(exp));
		}
		Number tetrate(const Real& exp, const Real& layer) const {
			return tetrate(Number(exp), Number(layer));
		}

		[[nodiscard]] Number pow_sum(const Number& other) const;

		[[nodiscard]] Number floor() const;

		// ---------- Inverse ----------
		[[nodiscard]] Number inv() const {
			Real d = abs_sq();
			if (d.is_zero()) return Number();
			return Number(r / d, (ZERO - i) / d);
		}

		// ---------- Display ----------
		[[nodiscard]] std::string toString() const;
		[[nodiscard]] std::string _to_String_short() const;   // 2 decimals (finite) or compact form (huge).

		// ---------- Trigonometry ----------
		[[nodiscard]] Number sin_of() const;
		[[nodiscard]] Number cos_of() const;
		[[nodiscard]] Number tan_of() const;
		[[nodiscard]] Number cot_of() const;
		[[nodiscard]] Number sec_of() const;
		[[nodiscard]] Number csc_of() const;

};

template<typename T>
Number _to_Number (T x) {
	if constexpr (std::is_same_v<T, Number>) {
		return x;
	} else if constexpr (std::is_same_v<T, Real>) {
		return Number(x, ZERO);
	} else if constexpr (std::is_arithmetic_v<T>) {
		if (x == 0 || std::isnan((long double)x)) return Number();
		if (x < 0) {
			long double a = -(long double)x;
			if (std::isinf(a)) return Number(Real(false, -1, 10.0L, (long double)UINT64_MAX), ZERO);
			return Number(Real(true, -1, a, 0.0L).normalize(), ZERO);
		}
		long double a = (long double)x;
		if (std::isinf(a)) return Number(Real(false, 1, 10.0L, (long double)UINT64_MAX), ZERO);
		return Number(Real(true, 1, a, 0.0L).normalize(), ZERO);
	} else {
		return Number();
	}
}

bool Number::is_negative() const {
	return this->r.is_negative();
}

bool Number::is_positive() const {
	return this->r.is_positive();
}

inline Number operator+(const Number& a, const Real& b) {
	return a + Number(b);
}
inline Number operator-(const Number& a, const Real& b) {
	return a - Number(b);
}
inline Number operator*(const Number& a, const Real& b) {
	return a * Number(b);
}
inline Number operator/(const Number& a, const Real& b) {
	return a / Number(b);
}

// Real arithmetic type overloads
inline Real operator+(long double a, const Real& b) {
	return Real(a) + b;
}
inline Real operator-(long double a, const Real& b) {
	return Real(a) - b;
}
inline Real operator*(long double a, const Real& b) {
	return Real(a) * b;
}
inline Real operator/(long double a, const Real& b) {
	return Real(a) / b;
}
inline Real operator+(const Real& a, long double b) {
	return a + Real(b);
}
inline Real operator-(const Real& a, long double b) {
	return a - Real(b);
}
inline Real operator*(const Real& a, long double b) {
	return a * Real(b);
}
inline Real operator/(const Real& a, long double b) {
	return a / Real(b);
}
// Real comparison with arithmetic types
inline bool operator==(long double a, const Real& b) {
	return Real(a) == b;
}
inline bool operator!=(long double a, const Real& b) {
	return Real(a) != b;
}
inline bool operator<(long double a, const Real& b) {
	return Real(a) < b;
}
inline bool operator>(long double a, const Real& b) {
	return Real(a) > b;
}
inline bool operator<=(long double a, const Real& b) {
	return Real(a) <= b;
}
inline bool operator>=(long double a, const Real& b) {
	return Real(a) >= b;
}
inline bool operator==(const Real& a, long double b) {
	return a == Real(b);
}
inline bool operator!=(const Real& a, long double b) {
	return a != Real(b);
}
inline bool operator<(const Real& a, long double b) {
	return a < Real(b);
}
inline bool operator>(const Real& a, long double b) {
	return a > Real(b);
}
inline bool operator<=(const Real& a, long double b) {
	return a <= Real(b);
}
inline bool operator>=(const Real& a, long double b) {
	return a >= Real(b);
}
// Number arithmetic type overloads
inline Number operator+(long double a, const Number& b) {
	return Number(a) + b;
}
inline Number operator-(long double a, const Number& b) {
	return Number(a) - b;
}
inline Number operator*(long double a, const Number& b) {
	return Number(a) * b;
}
inline Number operator/(long double a, const Number& b) {
	return Number(a) / b;
}
inline Number operator+(const Number& a, long double b) {
	return a + Number(b);
}
inline Number operator-(const Number& a, long double b) {
	return a - Number(b);
}
inline Number operator*(const Number& a, long double b) {
	return a * Number(b);
}
inline Number operator/(const Number& a, long double b) {
	return a / Number(b);
}
// Number comparison with arithmetic types
inline bool operator==(long double a, const Number& b) {
	return Number(a) == b;
}
inline bool operator!=(long double a, const Number& b) {
	return Number(a) != b;
}
inline bool operator<(long double a, const Number& b) {
	return Number(a) < b;
}
inline bool operator>(long double a, const Number& b) {
	return Number(a) > b;
}
inline bool operator<=(long double a, const Number& b) {
	return Number(a) <= b;
}
inline bool operator>=(long double a, const Number& b) {
	return Number(a) >= b;
}
inline bool operator==(const Number& a, long double b) {
	return a == Number(b);
}
inline bool operator!=(const Number& a, long double b) {
	return a != Number(b);
}
inline bool operator<(const Number& a, long double b) {
	return a < Number(b);
}
inline bool operator>(const Number& a, long double b) {
	return a > Number(b);
}
inline bool operator<=(const Number& a, long double b) {
	return a <= Number(b);
}
inline bool operator>=(const Number& a, long double b) {
	return a >= Number(b);
}
// Number-Real mixed overloads
inline Number operator+(const Real& a, const Number& b) {
	return Number(a) + b;
}
inline Number operator-(const Real& a, const Number& b) {
	return Number(a) - b;
}
inline Number operator*(const Real& a, const Number& b) {
	return Number(a) * b;
}
inline Number operator/(const Real& a, const Number& b) {
	return Number(a) / b;
}
inline bool operator==(const Real& a, const Number& b) {
	return Number(a) == b;
}
inline bool operator!=(const Real& a, const Number& b) {
	return Number(a) != b;
}
inline bool operator<(const Real& a, const Number& b) {
	return Number(a) < b;
}
inline bool operator>(const Real& a, const Number& b) {
	return Number(a) > b;
}
inline bool operator<=(const Real& a, const Number& b) {
	return Number(a) <= b;
}
inline bool operator>=(const Real& a, const Number& b) {
	return Number(a) >= b;
}

#define NARITH_OP(OP)                                                             \
	template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>      \
	inline Number operator OP (const Number& a, T b) {                            \
		return a OP Number(_to_Real(b));                                          \
	}                                                                             \
	template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>      \
	inline Number operator OP (T a, const Number& b) {                            \
		return Number(_to_Real(a)) OP b;                                          \
	}

NARITH_OP(+)
NARITH_OP(-)
NARITH_OP(*)
NARITH_OP(/)

#undef NARITH_OP

inline Number cplx_pow(const Real& base, const Number& exp) {
	return Number(base).pow(exp);
}
inline Number cplx_log(const Real& base, const Number& exp) {
	return Number(base).log(exp);
}

static inline Number realify(const Number& z) {
    if (z.i.is_zero()) return z;
    Real mag = z.r.abs();
    if (z.r.is_zero()) mag = z.i.abs();
    if (mag < ONE) mag = ONE;
    Real tol = mag * _to_Real(1e-9L);
    if (z.i.abs() <= tol) return Number(z.r, ZERO);
    return z;
}

Number Number::normalize() const {
	return realify(Number(r.normalize(), i.normalize()));
}

void Number::normalize_inplace() {
	r.normalize_inplace();
	i.normalize_inplace();
}

long double Number::_to_long_double() const {
	return this->r._to_long_double();
}

inline Real _to_Real_mod(const Number& z) {
	return z.abs();
}

const Number _ZERO = Number(ZERO);
const Number _ONE = Number(ONE);
const Number _TEN = Number(TEN);

const Number M_E      = Number(_to_Real<Real>(LD_E),    ZERO);
const Number M_PI     = Number(_to_Real<Real>(LD_PI),   ZERO);
const Number M_PI_2   = Number(_to_Real<Real>(LD_PI_2), ZERO);
const Number M_LN10   = Number(_to_Real<Real>(LD_LN10), ZERO);

const Number M_I      = Number(ZERO, ONE);
const Number M_I_NEG  = Number(ZERO, ZERO - ONE);

// atan of a Real (clamped via long double; atanl(inf) = pi/2)
static Real atan_real(const Real& x) {
	long double v = x._to_long_double();
	return _to_Real(atanl(v));
}

Real Number::arg() const {
	// axis cases
	if (i.is_zero()) {
		return r.is_negative() ? _to_Real(M_PI) : ZERO;
	}
	if (r.is_zero()) {
		Real pi_2 = _to_Real(M_PI_2);
		return i.is_negative() ? (ZERO - pi_2) : pi_2;
	}

	bool y_neg = i.is_negative();
	bool x_neg = r.is_negative();
	Real ya = i;
	if (y_neg) ya.neg = 1;
	Real xa = r;
	if (x_neg) xa.neg = 1;

	// compute atan of the smaller/larger ratio for numerical stability
	Real base;
	if (ya > xa) {
		Real ratio = xa / ya;
		base = _to_Real(M_PI_2) - atan_real(ratio);
	} else {
		Real ratio = ya / xa;
		base = atan_real(ratio);
	}

	if (x_neg) {
		Real pi = _to_Real(M_PI);
		if (y_neg) base = base - pi;
		else       base = pi - base;
	} else if (y_neg) {
		base = ZERO - base;
	}
	return base;
}

// e^z
Number Number::exp_of() const {
	if (i.is_zero()) {
		return Number(M_E.pow(r));
	}
	Real ea = _to_Real(M_E).pow(r);
	long double bl = i._to_long_double();
	Real cb, sb;
	if (std::isfinite(bl)) {
		cb = _to_Real(cosl(bl));
		sb = _to_Real(sinl(bl));
	} else {
		// |b| so large the phase is meaningless
		cb = ZERO;
		sb = ZERO;
	}
	return Number(ea * cb, ea * sb);
}

// ln(z) = ln|z| + iarg(z)
Number Number::ln_of() const {
	if (is_zero()) return Number();
	Real mod    = abs();
	Real ln_mod = Real::log10_of(mod) * _to_Real(M_LN10);
	Real theta  = arg();
	return Number(ln_mod, theta);
}

// z^w = e^(w ln z)
Number Number::pow(const Number& exp) const {
    Number b = realify(*this), e = realify(exp);
    if (b.is_real() && e.is_real()) return Number(b.r.pow(e.r));
    if (b.is_zero()) { if (e.is_zero()) return Number(ONE); return Number(); }
    return (e * b.ln_of()).exp_of();
}

// log_base(z) = ln(z) / ln(base)
Number Number::log(const Number& base) const {
	if (is_zero() || base.is_zero() || base == Number(ONE)) return Number();
	return ln_of() / base.ln_of();
}

// Dilation: z^^w = 10^(log10(z)^w)
Number Number::dilate(const Number& exp) const {
	if (this->abs() <= 10.0L) return *this;

	Number ten( _TEN );                 // Number(10 + 0i)
	Number log10_z = this->log(ten);    // log base 10
	return ten.pow(log10_z.pow(exp));
}

// Tetration: z^^^w = 10^(10^(log10(log10(z))^w))
Number Number::tetrate(const Number& exp) const {
	if (this->abs() <= 1e10L) return *this;

	Number ten( _TEN );
	Number log10_log10_z = this->log(ten).log(ten);
	return ten.pow( ten.pow( log10_log10_z.pow(exp) ) );
}

// Tetration
//   layer = 1  Dilation
//   layer = 2  Tetration
//   layer = 3  Pentation  ...
Number Number::tetrate(const Number& exp, const Number& layer) const {
	Real L = layer.r;

	Number aa = *this;
	Number a  = *this;
	Number ten( _TEN );

	for (Real k = ONE; k <= L; k = k + ONE) {
		a = a.log(ten);
		if (a.is_zero()) return aa;
	}

	a = a.pow(exp);

	for (Real k = ONE; k <= L; k = k + ONE) {
		a = ten.pow(a);
	}

	return a;
}

Number Number::pow_sum(const Number& other) const {
	Number nn = this->normalize();
	Number ss = other.normalize();

	if (nn.is_negative() || ss.is_zero()) return Number(_ONE);
	if (nn.is_zero() || nn.is_one())       return Number(_ONE);

	Number shifted = nn + Number(_to_Real(0.5L));
	Number exp_one = ss + Number(_ONE);

	if (exp_one.is_zero()) {
		return shifted.log(Number(_TEN));
	}

	Number result = shifted.pow(exp_one) / exp_one;

	if (result.is_negative()) result = Number();

	return result.normalize();
}

Number Number::floor() const {
	Real fr = ::floor(this->r);
	Real fi = ::floor(this->i);
	return Number(fr, fi);
}

inline Number floor(const Number& z) {
	return Number(::floor(z.r), ::floor(z.i));
}

// Construction of polar coordinate.
static Number polar(const Real& mod, const Real& theta) {
	long double t = theta._to_long_double();
	if (!std::isfinite(t)) {
		return Number(mod, ZERO);
	}
	Real c = _to_Real(cosl(t));
	Real s = _to_Real(sinl(t));
	return Number(mod * c, mod * s);
}

Number Number::sin_of() const {
	if (i.is_zero()) {
		long double v = r._to_long_double();
		if (!std::isfinite(v)) return Number();
		return Number(sinl(v));
	}
	Real eb  = real_exp(i);            // e^b
	Real enb = real_exp(ZERO - i);     // e^-b
	Real two = _to_Real(2);
	Real sh  = (eb - enb) / two;       // sinh(b)
	Real ch  = (eb + enb) / two;       // cosh(b)

	long double a = r._to_long_double();
	Real sa, ca;
	if (std::isfinite(a)) {
		sa = _to_Real(sinl(a));
		ca = _to_Real(cosl(a));
	} else {
		sa = ZERO;
		ca = ZERO;
	}
	return Number(sh * ca, ch * sa);
}

Number Number::cos_of() const {
	if (i.is_zero()) {
		long double v = r._to_long_double();
		if (!std::isfinite(v)) return Number();
		return Number(cosl(v));
	}
	Real eb  = real_exp(i);
	Real enb = real_exp(ZERO - i);
	Real two = _to_Real(2);
	Real sh  = (eb - enb) / two;
	Real ch  = (eb + enb) / two;

	long double a = r._to_long_double();
	Real sa, ca;
	if (std::isfinite(a)) {
		sa = _to_Real(sinl(a));
		ca = _to_Real(cosl(a));
	} else {
		sa = ZERO;
		ca = ZERO;
	}

	return Number(ch * ca, ZERO - (sh * sa));
}

Number Number::tan_of() const {
	return sin_of() / cos_of();
}
Number Number::cot_of() const {
	return cos_of() / sin_of();
}
Number Number::sec_of() const {
	return Number(ONE) / cos_of();
}
Number Number::csc_of() const {
	return Number(ONE) / sin_of();
}

// Real = -1?
static bool real_is_neg_one(const Real& x) {
	return x.neg < 0 && x.sign && x.layer == 0 && x.val == 1.0L;
}

static std::string N2S_save(const Real& n) {
	Real a = n.normalize();
	std::ostringstream oss;
	oss << "N("
	    << std::fixed << std::setprecision(10) << a.val   << ","
	    << a.layer                                        << ","
	    << (a.sign ? 1 : 0)                               << ","
	    << a.neg                                          << ")";
	return oss.str();
}

std::string Number::toString() const {
	if (is_zero())  return "0";
	if (i.is_zero()) return r.toString();

	// Im. != 0
	bool r_zero     = r.is_zero();
	bool i_one      = i.is_one();
	bool i_neg_one  = real_is_neg_one(i);

	if (r_zero) {
		if (i_one)     return "i";
		if (i_neg_one) return "-i";
		// Pure Im.
		if (i.is_negative()) {
			Real pos = ZERO - i;
			return "-" + pos.toString() + "i";
		}
		return i.toString() + "i";
	}

	// r != 0 and i != 0
	std::string rs = r.toString();

	if (i_one)     return rs + " + i";
	if (i_neg_one) return rs + " - i";

	if (i.is_positive()) return rs + " + " + i.toString() + "i";

	Real pos = ZERO - i;
	return rs + " - " + pos.toString() + "i";
}

std::string Number::_to_String_short() const {
	// Compact display that keeps 2 decimal places for ordinary-magnitude values.
	// This Number type can hold astronomically large values (10^(10^...)); for those
	// a long-double conversion overflows to +inf, so we fall back to the full compact
	// toString() (scientific / power-tower form) instead of printing "inf".
	if (is_zero()) return "0.00";
	if (!i.is_zero()) return toString();          // complex: keep full form.
	long double v = this->_to_long_double();
	if (!std::isfinite(v)) return toString();      // too huge for fixed 2-decimals.
	return format_with_commas(v, 2);               // finite: 2 decimals with commas.
}

static std::string N2S_save(const Number& n) {
	return "C(" + N2S_save(n.r) + "," + N2S_save(n.i) + ")";
}

// ============================================================
// Section 3 - Time related items initialization.
// ============================================================

//Get time as millisecond.
long double _get_time() {
	auto now = std::chrono::steady_clock::now();
	auto duration = now.time_since_epoch();
	auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
	return microseconds / 1000.0L;
}


static Number flux_pool = ZERO;
static long double flux_elapsed = 0;
static const long double FLUX_DURATION = 10.0L;
//_accel refers to game speed by Time Flux.
static Number _accel = _ONE;

Number _time_flux_accel(Number s) {
	//Time flux speed-up calculation.
	long double dt = s._to_long_double();
	long double accel = 1.0L;
	if (!flux_pool.is_zero() && flux_elapsed < FLUX_DURATION) {
		long double pool_val = flux_pool._to_long_double();
		long double remaining = FLUX_DURATION - flux_elapsed;
		long double rate = pool_val / remaining;
		long double consumed = rate * dt;
		if (consumed > pool_val) consumed = pool_val;
		if (dt <= 0) {
			_accel = ONE;
			return ONE;
		}
		accel = 1.0L + consumed / dt;
		pool_val -= consumed;
		if (pool_val < 0) pool_val = 0;
		flux_pool = Real(true, pool_val, 0);
		flux_elapsed += dt;
	}

	_accel = accel;
	return _accel;
}

// Converts time (x seconds) to a string. Used in Time Flux tab.
// 3608.96 -> "1:00:08.960"
string _to_time(long double x) {
	if (x < 0) x = 0;

	long long total_ms = (long long)(x * 1000.0L + 0.5L);
	long long ms  = total_ms % 1000;
	long long total_sec = total_ms / 1000;
	long long sec = total_sec % 60;
	long long total_min = total_sec / 60;
	long long min = total_min % 60;
	long long hr  = total_min / 60;

	ostringstream oss;
	oss << hr << ":";
	if (min < 10) oss << "0";
	oss << min << ":";
	if (sec < 10) oss << "0";
	oss << sec << ".";
	if (ms < 100) oss << "0";
	if (ms < 10)  oss << "0";
	oss << ms;
	return oss.str();
}

// ============================================================
// Section 4 - Player and major functions initialization.
// ============================================================

// Start from here, the game cycle begins.
// There are pre_loaded numbers for layout() to call instead if calculating
// the same thing again.
Number PF = TEN;
Number GIM = TEN;
Number PIM = TEN;
Number rate = ZERO;

// Structure for autobuyers.
struct atb {
	bool unlockable, unlocked, enabled;
};

// Public structure for an upgrade.
// ID map:
// 0-7: Amplifier 1-8 (A1-A8)
// 8-15: Intensifier 1-8 (I1-I8)
// 16-19: Prestige Upgrades (PU1-PU4)
// 20-21: Amp. and Int. Aeromagnifiers.
// 22-25: AUs' levels.
// 26-29: AUP bought.
// 30-39: BAUs.
// 40-55: AA,AI.
// 56-57: AAA,AIA.
// 58-73: BA/BI.
// 74,75: BAA/BAI.
// 76- : Beta Upgrades.
struct upg_ {
	Number level = ZERO;
	atb autobuyer;
	Number init_cost = ZERO;
	Number scale = ZERO;
};

// Details in usage.
struct opt_	{
	Number val;
	string val_s;
};

// Items in pre-alpha stage.
// player.pre_alpha.highest is used to calculate GIM reduction.
struct pre_alpha_ {
	Number score = ONE; 											// Matter.
	Number amount[8] = {ONE,ONE,ONE,ONE,ONE,ONE,ONE,ONE};	        // M1-M8.
	Number highest = ZERO;										    // For prestige->GIM.
	Number time = 0; // How much time has passed since last prestige, etc.
};

struct ball_ {
	Number value;
	Number x, y;
	Number spawn = Number(1000);   // value the ball was spawned with (ball color ratio)
	double vx = 0.0, vy = 0.0;   // ball drift velocity (px/s, virtual board; not saved)

	bool is_alive() const {
		double v = (double)value._to_long_double();
		return std::isfinite(v) && v > 0.0;
	}
	pair <Number, Number> _get_pos() {
		return {x, y};
	}
};


// Precision guard for the 10-to-1 Tier synthesis gate. A drop count that
// displays as "10.000000" can sit a hair under 10 (e.g. 9.9999999) after
// accumulated float arithmetic, which would wrongly block synthesis. The
// tolerance (1e-6) matches the 6-decimal on-screen display, so anything that
// renders as "10.000000" is treated as a full 10.
static inline bool sim_has_ten(const Number& n) {
	return !(n < Number(10) - Number(1e-6));
}
struct simulator_ {
	upg_ level_drops[100][4];                 // Levels of drops: [0] is value, [1] is gravity and [2] is initial speed.
	// [3] is only used in [1][3] to calculate drops bought.
	// id starts from 1.
	upg_ level_global[2][3];                  // Three kinds of levels upgraded using Am ([0]) or AP ([1]).
	Number level = _ZERO;                     // Simulation level.
	Number score = _ONE;					  // (Simulation) Points.
	Number accumulate = _ZERO;                // Accumulated points (for Simulation level upgrade)
	Number* am_ref = nullptr;                 // -> player.alpha.score (Am). Global(Am) buys pay from here.
	Number* ap_ref = nullptr;                 // -> player.alpha.point (AP). Global(AP) buys pay from here.
	Number global_level_bonus = _ZERO;        // Extra Global levels from SL + Ach.47. Added to the 1.02^Global exponent. Set each frame in DrawAlphaSimulator.
	Number drop_value_mult    = _ONE;         // Ach.44 multiplier (+10% Drop value). Set each frame in DrawAlphaSimulator.
	ball_  ball[50];                          // States of balls.
	Number drops[100];                        // Numbers of drops at different Tiers.

	// ---- Live drop sprites: shared between update() physics and DrawAlphaSimulator render ----
	// When the total number of owned drops is below MAX_DROPS we simulate every single
	// drop (one sprite each); once the total reaches MAX_DROPS we only keep the
	// MAX_DROPS largest-Tier sprites (largest tiers first), capping both physics and render.
	enum { MAX_DROPS = 250 };                 // cap of live falling drop sprites
	double dx[MAX_DROPS];                     // sprite x (virtual board px)
	double dy[MAX_DROPS];                     // sprite y (virtual board px)
	double dvy[MAX_DROPS];                    // sprite vertical velocity (px/s)
	double dvx[MAX_DROPS];                    // sprite horizontal velocity (px/s)
	int    dtier[MAX_DROPS];              	  // sprite Tier (1..100), 0 = empty slot
	bool   dateat[MAX_DROPS];                   // ghost drop already consumed its one ball this downward pass
	bool   pool_init = false;             	  // one-time pool reset flag
	int    board_w_px = -1, board_h_px = -1;  // last rendered board pixel size (resize recycle)
	bool   drop_pool_dirty = false;           // set true on board resize -> recycle all drops

	// ---- Diagnostics: revealed by the on-screen overlay in DrawAlphaSimulator.
	// dbg_update_calls counts how often update() runs; dbg_last_dt is the time
	// step it received (0 -> physics frozen).
	long   dbg_update_calls = 0;
	double dbg_last_dt      = 0.0;
	
	bool _get_ach(int id);
	Number _get_SP_effect(int id);
	// Initialize the cost of upgrades.
	void cost_init() {
		for(int i = 1; i <= 100; i ++) {
			level_drops[i][0].init_cost = Number(100).pow(i - 1);
			level_drops[i][0].scale = 1.5 - (i - 1) / 400.0;
			level_drops[i][1].init_cost = Number(100).pow(i - 1);
			level_drops[i][1].scale = 1.5;
			level_drops[i][2].init_cost = Number(100).pow(i - 1);
			level_drops[i][2].scale = 1.5;
		}
		// Putchase new drops.
		level_drops[1][3].init_cost = 1;
		level_drops[1][3].scale = 1.25;
	}
	// Returns the current cost of level_balls[id1][id2].
	Number _get_cost(int id1, int id2) {
		// Negative id1 => global upgrade line (level_global).
		// gidx 0 = Am-spend line, gidx 1 = AP-spend line; id2 = which kind (value/gravity).
		if (id1 < 0) {
			int gidx = (-id1) - 1;
			if (gidx < 0) gidx = 0;
			if (gidx > 1) gidx = 1;
			Number g = level_global[gidx][id2].level;          // already bought count for this kind
			Number base  = Number(10).pow(20);                 // Both lines start at 1e20. Am rises x1e5/level (1e20,1e25,1e30,1e35...), AP x1e2/level.
			Number ratio = Number(10).pow(gidx == 0 ? 5 : 2); // 1e5 (Am) or 1e2 (AP)
			return base * ratio.pow(g);
		}
		Number x;
		if(id2 <= 2) x = level_drops[id1][id2].level;
		else {
			// Calculates ball amounts.
			for(int i = 1; i <= 100; i ++) x += Number(10).pow(i - 1) * drops[i];
		}
		Number s = level_drops[id1][id2].scale;
		Number i = level_drops[id1][id2].init_cost;
		// Buying NEW drops (id2 == 3) now grows as a geometric sequence
		if (id2 == 3) {
			double xd = x._to_long_double();
			if (std::isfinite(xd)) {
				double e = xd - 1.0;
				if (e < 0.0) e = 0.0;
				return i * s.pow(Number(e));
			}
			// Extreme drop counts (x overflows double): keep the complex exponent, floored at 0.
			Number e3 = (x > ONE) ? (x - ONE) : Number(0);
			return i * s.pow(e3);
		}
		return i * s.pow(x * x / 2 - x / 2);
	}
	// Returns the current cost of upgrading Simulation level.
	Number _get_cost() {
		Number x = level + ONE;
		if(x <= TEN) return 100 * x.pow(x);
		else{
			return 100 * x.pow(x - 0.5 * (x - 10));
		}
	}
	// Returns the value (damage/collision) of Drop id.
	Number _get_val(int id) {
		Number a = Number(20).pow(id - 1) * (ONE + 0.05 * level_drops[id][0].level) * (Number(1.05).pow((level_global[0][0].level + level_global[1][0].level + global_level_bonus))) * drop_value_mult;
		if(_get_ach(42)) a = a * 1.5;
		if(_get_ach(50)) a = a * (level + ONE);   // Ach.50: Drop value x (SL+1)
		return a;
	}
	// Returns the gravity (affects speed) of Drop id.
	Number _get_gravity(int id) {
		Number a = (ONE + Number(0.05) * level_drops[id][1].level) * (Number(1.05).pow((level_global[0][1].level + level_global[1][1].level + global_level_bonus)));
		if(_get_ach(44)) a = a * ((25 + level).pow(0.5) - 4);
		if(_get_ach(46)) a = a * 2;
		return a;
	}
	// Returns the initial speed (affects speed) of Drop id.
	Number _get_init_speed(int id) {
		Number a = (ONE + Number(0.05) * level_drops[id][2].level) * (Number(1.05).pow((level_global[0][1].level + level_global[1][1].level + global_level_bonus)));
		if(_get_ach(42)) a = a * 1.5;
		return a;
	}
	// Trying to upgrade Tier id1's upgrade id2 once.
	// If id1 is negative, that means it's upgrading global upgrades (level_global).
	// Returns true if upgrade is successful.
	// A drop whose gravity OR initial speed exceeds 100 becomes a "ghost" drop:
	// it skips real collision physics AND never touches balls; instead it banks the
	// FULL SP of its own value the moment it reaches the bottom, then keeps
	// falling straight down and ghosts through every other ball. Rendered
	// semi-transparent. Keeps SP flowing even when high gravity makes physical
	// collisions hard to land.
	bool _is_fast_drop(int id) {
		if (_get_gravity(id) > Number(100)) return true;
		if (_get_init_speed(id) > Number(100)) return true;
		return false;
	}
	
	bool _try_upgrade(int id1, int id2) {
		// Try to upgrade Tier id1's upgrade id2 once.
		// If id1 is negative, upgrade global upgrades (level_global).
		// Returns true if upgrade is successful.
		if (id1 < 0) {
			// Global upgrades: level_global[abs(id1)-1][id2]
			int gidx = (-id1) - 1;
			if (gidx < 0 || gidx >= 2) return false;
			// Global upgrades are paid with Am (gidx 0) or AP (gidx 1), NOT SP.
			Number* res = (gidx == 0) ? am_ref : ap_ref;
			if (!res) return false;
			Number cost = _get_cost(id1, id2);
			if (*res < cost) return false;
			*res = (*res - cost).normalize();
			level_global[gidx][id2].level += 1;
			return true;
		}
		// Regular drop upgrades: level_drops[id1][id2]
		if (id1 < 1 || id1 > 100) return false;
		if (id2 < 0 || id2 > 3) return false;
		// For id2 == 3 (drop tier purchase), check SM8 requirement
		if (id2 == 3 && id1 != 1) {
			// SM8: level >= 50 allows higher tier drops
			if (level < 50) return false;
		}
		// Special case: the FIRST Tier 1 drop is free. The board starts empty
		// (drops[1] == 0), so the very first purchase must succeed without any
		// SP cost; otherwise a player holding 0 SP can never bootstrap the run.
		// Every later Tier 1 drop still goes through the normal cost gate below.
		if (id1 == 1 && id2 == 3 && drops[1] <= ZERO) {
			drops[1] += 1;
			return true;
		}
		Number cost = _get_cost(id1, id2);
		if (score < cost) return false;
		if (id2 == 3) {
			// id2 == 3 is the "acquire a drop" action.
			if (id1 == 1) {
				// Buy ONE Tier 1 drop, paying SP (_get_cost(1,3)).
				// Old code wrongly required 16 existing Tier 1 drops, so the
				// first Tier 1 drop could never be bought even with SP >= cost.
				score -= cost;
				drops[1] += 1;
			} else {
				// Merge: 10 of Tier (id1-1) -> 1 of Tier id1 (in-kind, no SP).
				if (!sim_has_ten(drops[id1 - 1])) return false;
				drops[id1 - 1] -= 10;
				drops[id1]     += 1;
			}
			return true;
		}
		// value / gravity / init-speed upgrades: pay SP then level up.
		score -= cost;
		level_drops[id1][id2].level += 1;
		return true;

	}
	// ---- Tier synthesis: 10 of Tier id1 -> 1 of Tier (id1+1). ----
	// Pure in-kind conversion. UNRESTRICTED: NOT gated by SM8 (SL>=50) and
	// costs NO SP. The only requirement is owning >= 10 of Tier id1.
	// Returns true if exactly one synthesis happened.
	bool _try_synthesize(int id1) {
		if (id1 < 1 || id1 >= 100) return false;
		if (!sim_has_ten(drops[id1])) return false;
		drops[id1]     -= 10;
		drops[id1 + 1] += 1;
		return true;
	}
	// Keep synthesizing while at least 10 Tier id1 remain (Buy Max path).
	bool _try_synthesize_bulk(int id1) {
		bool changed = false;
		while (_try_synthesize(id1)) changed = true;
		return changed;
	}
	// Trying to upgrade Tier id1's upgrade id2 with all possible resources.
	// If id1 is negative, that means it's upgrading global upgrades (level_global).
	// Returns true if the level is changed.
	// NOTE: If id1 != 1 and id2 = 3 and SM 8 is not unlocked, do nothing.
	// Otherwise, upgrade while only editing level_drops[1][3], for example,
	// buying 1 Tier 2 Drops is equivalent to buying 10 Tier 1 Drop.
	bool _try_upgrade_bulk(int id1, int id2) {
		// Try to upgrade Tier id1's upgrade id2 with all possible resources.
		// If id1 is negative, upgrade global upgrades (level_global).
		// Returns true if the level changed.
		// NOTE: If id1 != 1 and id2 = 3 and SM 8 is not unlocked, do nothing.
		if (id1 != 1 && id2 == 3) {
			// SM8 requirement: simulation level >= 50
			if (level < 50) return false;
		}
		bool changed = false;
		while (true) {
			if (!_try_upgrade(id1, id2)) break;
			changed = true;
		}
		return changed;

	}
	// Trying to upgrade Simulation level.
	// Returns true if upgrade is successful.
	bool _try_upgrade() {
		// Try to upgrade Simulation level.
		// Returns true if upgrade is successful.
		// SP needed for SL x upgrade is 100 * x^x.
		Number cost = _get_cost();
		if (accumulate < cost) return false;
		accumulate -= cost;
		accumulate = ZERO;
		level += 1;
		return true;
	}
	// Updating real time data & drop positions, etc. This is part of the game loop function.
	void update(Number s) {
		// ============================================================
		// Alpha Simulator game loop (tech doc 4.3.4.2).
		// Runs every frame while the simulator is unlocked. Physics runs
		// in a fixed virtual board space (W x H); DrawAlphaSimulator maps
		// these coordinates onto the actual ImGui child rectangle.
		//   * 10 balls on board, value based on Simulation level (no gravity)
		//   * drops fall by innate initial speed + gravity
		//   * drop touches ball -> ball.value -= drop.value, gain SP
		//   * ball.value < drop.value -> ball destroyed, respawn at random place
		//   * drop leaves bottom -> destroyed, new drop spawns at top
		//   * bouncing halves the drop speed
		// ============================================================
		dbg_update_calls++;
		dbg_last_dt = (double)s._to_long_double();
		if (!std::isfinite(dbg_last_dt) || dbg_last_dt < 0.0) dbg_last_dt = 0.0;
		Number dt = s;                 // time step (seconds)
		const double W = 800.0, H = 800.0;   // virtual board size (px) -- taller for more vertical room
		int    N_BALLS   = 10;               // balls kept alive on the board
		if(_get_ach(45)) N_BALLS = 25;
		if(_get_ach(49)) N_BALLS = 50;
		const double BALL_R    = 30.0;       // ball collision radius
		const double DROP_R    = 10.0;       // drop collision radius
		const int    MAX_DROPS = simulator_::MAX_DROPS;  // live drop sprite cap (250)
		// --- Ball init / auto-fill destroyed balls ---
		// Done BEFORE the dt<=0 guard and using float-safe checks: balls are always
		// seeded even if effective_s was clamped to ZERO upstream (which made update()
		// return early and left every ball at value 0 -> empty board), and WITHOUT
		// relying on the complex-number operator<=/> of the Number type.
		for (int i = 0; i < N_BALLS; i++) {
			if (!ball[i].is_alive()) {
				// Ball value scales with Simulation level (real members, no player.*).
				ball[i].value = Number(10) * Number(2).pow(level);   // life = 10 * 2^(Simulation level)
				ball[i].spawn = ball[i].value;
				// Spread across the WHOLE board with a BALL_R margin so a circle
				// never renders outside the board (old code clustered to centre).
				double rx = BALL_R + ((double)(rand() % 10000) / 10000.0) * (W - 2.0 * BALL_R);
				double ry = BALL_R + ((double)(rand() % 10000) / 10000.0) * (H - 2.0 * BALL_R);
				ball[i].x = Number(rx);
				ball[i].y = Number(ry);
			}
		}
		if (dt <= ZERO) return;        // balls are seeded; physics frozen this tick
		// --- Ball drift motion + wall bounce across the WHOLE board ---
		{
			double dt_d0 = dt._to_long_double();
			if (!std::isfinite(dt_d0) || dt_d0 < 0.0) dt_d0 = 0.0;
			for (int i = 0; i < N_BALLS; i++) {
				if (!ball[i].is_alive()) continue;
				double bx = (double)ball[i].x._to_long_double();
				double by = (double)ball[i].y._to_long_double();
				if (!std::isfinite(bx)) bx = W * 0.5;
				if (!std::isfinite(by)) by = H * 0.5;
				ball[i].x = Number(bx);
				ball[i].y = Number(by);
			}
		}

		// --- First-time sprite pool reset ---
		if (!pool_init) {
			for (int i = 0; i < MAX_DROPS; i++) {
				dtier[i] = 0;
				dx[i] = 0.0;
				dy[i] = 0.0;
				dvy[i] = 0.0;
				dateat[i] = 0;
				dvx[i] = 0.0;
			}
			pool_init = true;
		}
		// On a window / board resize, wipe the whole live-sprite pool so the reconciler
		// re-seeds every drop fresh at the top for the new extent.
		if (drop_pool_dirty) {
			for (int i = 0; i < MAX_DROPS; i++) {
				dtier[i] = 0; dx[i] = 0.0; dy[i] = 0.0; dvx[i] = 0.0; dvy[i] = 0.0; dateat[i] = 0;
			}
			drop_pool_dirty = false;
		}

		// --- Reconcile the live sprite pool with the desired multiset ---
		//   * total owned drops <  MAX_DROPS : simulate EVERY drop (one sprite each).
		//   * total owned drops >= MAX_DROPS : keep only the MAX_DROPS largest-Tier sprites.
		// Existing sprites keep their positions/velocities; we only spawn missing sprites
		// (at the top of the board) or free surplus sprites of an over-represented Tier.
		{
			int target[101];
			int alive[101];
			for (int t = 0; t <= 100; t++) {
				target[t] = 0;
				alive[t] = 0;
			}

			// Count total owned drops, saturating at MAX_DROPS (avoid huge Number -> int).
			long long total = 0;
			for (int t = 1; t <= 100; t++) {
				if (drops[t] <= ZERO) continue;
				long long c = (long long)drops[t]._to_long_double();
				if (c < 0) c = 0;
				total += c;
				if (total >= MAX_DROPS) {
					total = MAX_DROPS;
					break;
				}
			}

			if (total < MAX_DROPS) {
				// Per-sprite: exactly one sprite per owned drop.
				for (int t = 1; t <= 100; t++) {
					if (drops[t] <= ZERO) continue;
					long long c = (long long)drops[t]._to_long_double();
					if (c < 0) c = 0;
					if (c > MAX_DROPS) c = MAX_DROPS;
					target[t] = (int)c;
				}
			} else {
				// Largest-Tier first greedy fill up to MAX_DROPS sprites.
				int left = MAX_DROPS;
				for (int t = 100; t >= 1 && left > 0; t--) {
					if (drops[t] <= ZERO) continue;
					long long c = (long long)drops[t]._to_long_double();
					if (c < 0) c = 0;
					int take = (c >= (long long)left) ? left : (int)c;
					target[t] = take;
					left -= take;
				}
			}

			// How many sprites of each Tier are currently alive.
			for (int i = 0; i < MAX_DROPS; i++)
				if (dtier[i] >= 1 && dtier[i] <= 100) alive[dtier[i]]++;

			// Free surplus sprites of an over-represented Tier (set slot empty).
			for (int t = 1; t <= 100; t++) {
				int extra = alive[t] - target[t];
				if (extra <= 0) continue;
				for (int i = 0; i < MAX_DROPS && extra > 0; i++)
					if (dtier[i] == t) {
						dtier[i] = 0;
						extra--;
					}
			}
			// Spawn missing sprites (spawn at the top of the board at a random x).
			for (int t = 1; t <= 100; t++) {
				int need = target[t] - alive[t];
				if (need <= 0) continue;
				for (int i = 0; i < MAX_DROPS && need > 0; i++) {
					if (dtier[i] == 0) {
						dtier[i] = t;
						dateat[i] = 0;
						dx[i]  = (double)(rand() % (int)W);
						dy[i]  = 0.0;
						dvy[i] = (double)(_get_init_speed(t)._to_long_double()) * 60.0;
						if (!std::isfinite(dvy[i])) dvy[i] = 240.0;   // overflow guard
						dvx[i] = ((double)(rand() % 100) / 100.0) * 80.0 - 40.0;   // small horizontal drift [-40,40]
						if (!std::isfinite(dvx[i])) dvx[i] = 0.0;
						need--;
					}
				}
			}
		}

		// --- Physics + collisions (per live sprite) ---
		Number gained = ZERO;
		double dt_d = dt._to_long_double();
		// Iterate the REAL live-sprite pool (dx/dy/dvy/dtier). There is no
		// dropSprites array; dtier[i]==0 marks an empty slot.
		for (int i = 0; i < MAX_DROPS; i++) {
			int t = dtier[i];
			if (t < 1 || t > 100) continue;                 // empty slot
			bool fast = _is_fast_drop(t);                   // ghost drop: no real collision
			double oldx = dx[i], oldy = dy[i];              // pre-move pos for swept contact test
			// Fall under per-tier gravity, guarded against overflow / NaN.
			double grav = (double)_get_gravity(t)._to_long_double();
			if (!std::isfinite(grav)) grav = 1.0;
			dvy[i] += grav * 100.0 * dt_d;
			if (!std::isfinite(dvy[i])) dvy[i] = 240.0;
			dy[i]  += dvy[i] * dt_d;
			if (fast) {
				// Ghost drop falls in a straight vertical line: no drift, no wall bounce.
				dvx[i] = 0.0;
			} else {
				dx[i]  += dvx[i] * dt_d;
				if (!std::isfinite(dx[i])) { dx[i] = W * 0.5; dvx[i] = 0.0; }
				if (dx[i] < DROP_R)      { dx[i] = DROP_R;      dvx[i] =  std::fabs(dvx[i]); }
				if (dx[i] > W - DROP_R)  { dx[i] = W - DROP_R;  dvx[i] = -std::fabs(dvx[i]); }
			}
			// A drop that reaches a ball BOUNCES off it (it no longer disappears and
			// regenerates at the top). The sprite is pushed just outside the ball along
			// the collision normal and its velocity is reflected upward; because it is
			// separated past the radius in the same frame it stops overlapping, so value/
			// SP is paid once per discrete bounce, not every frame (no "SP +1" regression).
			if (!fast)
			for (int j = 0; j < N_BALLS; j++) {
				if (!ball[j].is_alive()) continue;
				double bx = (double)ball[j].x._to_long_double();
				double by = (double)ball[j].y._to_long_double();
				double ddx = dx[i] - bx, ddy = dy[i] - by;
				double d2 = ddx * ddx + ddy * ddy, rr = BALL_R + DROP_R;
				if (d2 < rr * rr) {
					Number dval = _get_val(t);
					ball[j].value = (ball[j].value - dval).normalize();
					gained = gained + dval;
					// --- Bounce off the ball instead of recycling it ---
					double dist = std::sqrt(d2);
					if (!(std::isfinite((double)dist)) || dist < 1e-6) dist = 1e-6;   // coincident-centre guard
					double nx = ddx / dist, ny = ddy / dist;    // collision normal: ball centre -> sprite
					const double SEP = 1.02;                    // push sprite a hair past the surface
					dx[i] = bx + nx * rr * SEP;                 // separate so overlap ends this frame
					dy[i] = by + ny * rr * SEP;
					const double REST = 0.7;                    // restitution (bounciness)
					// Reflect the velocity vector about the collision normal so the drop can
					// bounce sideways (along the normal), NOT just straight up.
					double vdotn = dvx[i] * nx + dvy[i] * ny;   // velocity projected onto the normal
					if (vdotn < 0.0) {                          // moving INTO the ball -> mirror it out
						dvx[i] -= (1.0 + REST) * vdotn * nx;    // sideways (normal-x) kick
						dvy[i] -= (1.0 + REST) * vdotn * ny;    // vertical (normal-y) kick
					}
					dvx[i] += (double)((rand() % 41) - 20) * 2.0;   // tangential jitter so hits scatter
					if (!std::isfinite(dvx[i])) dvx[i] = 0.0;
					if (!std::isfinite(dvy[i])) dvy[i] = -200.0;
					long double bv = ball[j].value._to_long_double();
					if (!(std::isfinite((double)bv) && bv > 0.0L)) {   // ball destroyed
						ball[j].value = Number(10) * Number(2).pow(level);   // respawn life = 10 * 2^(SL)
						ball[j].spawn = ball[j].value;
						ball[j].x = Number(BALL_R + ((double)(rand() % 10000) / 10000.0) * (W - 2.0 * BALL_R));
						ball[j].y = Number(BALL_R + ((double)(rand() % 10000) / 10000.0) * (H - 2.0 * BALL_R));
						ball[j].vx = ((double)(rand() % 100) / 100.0) * 120.0 - 60.0;   // fresh drift on respawn
						ball[j].vy = ((double)(rand() % 100) / 100.0) * 120.0 - 60.0;
					}
					break;
				}
			}
			if (dtier[i] == 0) continue;                    // killed by a ball above
			// Left the bottom -> recycle at top with a fresh speed (no double SP).
			if (dy[i] > H + DROP_R) {
				if (fast && !dateat[i]) {
					gained = gained + _get_val(t);
					dateat[i] = true;
				}
				dy[i] = -DROP_R;
				dx[i] = (double)(rand() % (int)W);
				double iv = (double)_get_init_speed(t)._to_long_double();
				if (!std::isfinite(iv)) iv = 240.0;
				dvy[i] = iv * 60.0;
				dvx[i] = ((double)(rand() % 100) / 100.0) * 80.0 - 40.0; dateat[i] = 0;
			}
			// A drop that flies back above the TOP border is recycled (slot freed); the
			// reconciler respawns a fresh sprite at the top on the next tick.
			if (dy[i] < -DROP_R) dtier[i] = 0;
		}


		// --- Bank the SP gained this tick ---
		gained = gained * _get_SP_effect(1);
		if(_get_ach(43)) gained = gained * (level + 1);
		// Ach.51: SP x log10(AP); Ach.52: SP x log10(Am).
		if(_get_ach(51) && ap_ref != nullptr) { Number _l51 = (*ap_ref).log(TEN); if(_l51 >= ONE) gained = gained * _l51; }
		if(_get_ach(52) && am_ref != nullptr) { Number _l52 = (*am_ref).log(TEN); if(_l52 >= ONE) gained = gained * _l52; }
		score      = (score      + gained).normalize();
		accumulate = (accumulate + gained).normalize();

		// --- Shift+1 hotkey: Buy a Tier 1 drop (README) ---
#ifdef _WIN32
		{
			bool held = (GetAsyncKeyState(VK_SHIFT) & 0x8000) && (GetAsyncKeyState('1') & 0x8000);
			static bool prev_held = false;
			if (held && !prev_held) _try_upgrade(1, 3);   // buy on key-down only
			prev_held = held;
		}
#endif
	}
};

struct alpha_ {
	Number init_second_scale = 4;             // The second scale used to calculate cost.
	Number highest = ZERO;					  // Highest in-Alpha matter reached.
	Number point = ZERO;					  // Alpha Points.(or AP in short.)
	Number alpha = ZERO;					  // Alpha you have. Notice that alpha isn't AP.
	// It more represents how many times you (performed) alpha, later alpha/time may increase.
	int tab = 1;							  // Which tab you are in.
	bool autobuyer = false; 				  // Whether Alpha autobuyer (From Alpha Milestone 7, 20 Alpha) is enabled.
	Number time = 0;                          // How much time has passed since last alpha, etc.

	simulator_ simulator;

	bool unlocked_simulator = false;

	// === Alpha Amplifier / Alpha Intensifier system ===
	Number score    = ONE;      // Am (Alpha matter)
	Number am_best  = ONE;      // Highest Am.
	Number am_rate  = ZERO;     // Current Am rate.
	Number AIM      = TEN;      // Alpha Instability Magnitude.
	Number amount[8] = {ONE,ONE,ONE,ONE,ONE,ONE,ONE,ONE}; // AM1-AM8
};

struct beta_ {
	int tab = 1;							  // Which tab you are in.
	bool autobuyer = false; 				  // Whether Beta autobuyer (NYI) is enabled.
	Number time = 0;                          // How much time has passed since last beta, etc.

	Number highest = ZERO;					  // Highest in-Beta matter reached.
	Number highest_am = ZERO;				  // Highest in-Beta Alpha matter reached.
	Number highest_bm = ZERO;				  // Highest in-Beta Beta matter reached.
	Number point = ZERO;					  // Beta Points.
	Number beta = ZERO;					      // Beta you have.

	Number decay     = 0.99;                  // The base Beta Decay Number (BDN).
	Number decay_factor = 0.5;                // The Beta Decay Factor (BDF).
	Number score     = ONE;                   // Bm (Beta matter)
	Number bm_best  = ONE;                    // Highest Bm.
	Number bm_rate  = ZERO;                   // Current Bm rate.
	Number BIM      = TEN;                    // Beta Instability Magnitude.
	Number amount[8] = {_ONE ,_ONE ,_ONE ,_ONE ,_ONE ,_ONE ,_ONE ,_ONE};
	Number weights[4] = {_ZERO, _ZERO, _ZERO, _ZERO};
};

// Self-explanatory.
const string au_names[4] = {"(AU1) Improve Amp. Aero. effect.",
                            "(AU2) Improve Int. Aero. effect.",
                            "(AU3) Reduce GIM by extra 5% per level.",
                            "(AU4) Reduce second cost scale effect by 1%."
                           };
const string aup_names[4] = {"(AUP1) Buy AUP with matter.",
                             "(AUP2) Buy AUP with Amp. Aero.",
                             "(AUP3) Buy AUP with Int. Aero.",
                             "(AUP4) Buy AUP with AP."
                            };
const Number am[100] = {1, 2, 4, 8, 10, 15, 20};
const string am_name[100] = {
	" Alpha: Buying Aeromagnifiers no longer resets Amp. and Int.",
	" Alpha: Unlock Autobuyers for Amplifiers.",
	" Alpha: Unlock Autobuyers for Intensifiers.",
	" Alpha: Allow breaking Alpha (Gain AP based on matter).",
	" Alpha: Keep PUs on Alpha.",
	" Alpha: Unlock Autobuyers for Aeromagnifiers.",
	" Alpha: Unlock Autobuyer for Alpha."
};
const Number sm[100] = {3, 5, 8, 10, 12, 14, 16, 20};
const string sm_name[100] = {
	" Simulation Level: Apply Story 1 reward to Alpha Amplifiers.",
	" Simulation Level: Multiply Alpha matter gain by log10(Am).",
	" Simulation Level: Multiply Alpha matter gain by log10(AP).",
	" Simulation Level: Apply Ach. Reward to AIs.",
	" Simulation Level: ^5 to BAU8 effect.",
	" Simulation Level: Triple effect of BAU9.",
	" Simulation Level: AAs gain a mult. equal to log10(log10(matter)).",
	" Simulation Level: Now you can buy higher tier drops."
};
const string bau_names[20] = {
	"(BAU1) Multiply AP gain by x4.",
	"(BAU2) Multiplier to Amp. based on Alpha.",
	"(BAU3) Gain more Alpha based on matter.",
	"(BAU4) Passively gain AP at a rate multiplied by your fastest AP/min.",
	"(BAU5) Unlock the Alpha Amplifier / Alpha Intensifier system.",
	"(BAU6) Unlock Alpha Aeromagnifiers.",
	"(BAU7) Ach. multiplier now applies to Alpha Amplifier.",
	"(BAU8) Multiplier to Am generation based on AP.",
	"(BAU9) Reduction to AIM based on AP.",
	"(BAU10) Multiplier to AP gain based on Am."
};

const string bu_names[20] = {"(BU1) Gain 1 Beta Energy, which can be used to weaken Beta Decay."};
const Number bm[100] = {2, 3, 5, 8, 10, 12, 14, 16, 20, 30};
const string bm_name[100] = {
	" Beta: Keep Alpha Milestone 1 on Beta resets.",
	" Beta: Keep Alpha Milestone 2 on Beta resets.",
	" Beta: Keep Alpha Milestone 3 on Beta resets.",
	" Beta: Keep Alpha Milestone 4 on Beta resets.",
	" Beta: Keep Alpha Milestone 5 on Beta resets.",
	" Beta: Keep Alpha Milestone 6 on Beta resets.",
	" Beta: Keep Alpha Milestone 7 on Beta resets.",
	" Beta: Keep Break Alpha Upgrades on Beta resets.",
	" Beta: Keep Alpha Simulator on Beta resets.",
	" Beta: Autobuy AUPs and AUP now applies to every AU."
};
const string ach_name[1000] = {"",
                               "(Ach. 1) The Beginning",
                               "(Ach. 2) Empowerment",
                               "(Ach. 3) Back to linear",
                               "(Ach. 4) Quadritic",
                               "(Ach. 5) Gate to prestige",
                               "(Ach. 6) You PRESTIGED!",
                               "(Ach. 7) Gate to PU1",
                               "(Ach. 8) AEROMAGNIFIERS?!?",
                               "(Ach. 9) Autonomous Amplifier",
                               "(Ach. 10) Amplifier Apeirogon",
                               "(Ach. 11) The Fourth Episode",
                               "(Ach. 12) The Fourth Fate",
                               "(Ach. 13) Perks of living",
                               "(Ach. 14) Pentademic",
                               "(Ach. 15) Contingent",
                               "(Ach. 16) Finally not 10 again...",
                               "(Ach. 17) Double of root of all evil",
                               "(Ach. 18) 4 times of evil",
                               "(Ach. 19) Lucky one",
                               "(Ach. 20) ...maybe.",
                               "(Ach. 21) Rebirth",
                               "(Ach. 22) 90 degrees from infinity",
                               "(Ach. 23) -infinity^2",
                               "(Ach. 24) The limit of Aeros I",
                               "(Ach. 25) The limit of Aeros II",
                               "(Ach. 26) Aleph-0",
                               "(Ach. 27) Nice Strategy",
                               "(Ach. 28) Second confront",
                               "(Ach. 29) ...completed.",
                               "(Ach. 30) Breaking the limits",
                               "(Ach. 31) Breakthrough I",
                               "(Ach. 32) ANOTHER ONE",
                               "(Ach. 33) No PU consumptions",
                               "(Ach. 34) Perks of living II",
                               "(Ach. 35) Autonomous",
                               "(Ach. 36) Alpha amplification",
                               "(Ach. 37) No need for Caps + A",
                               "(Ach. 38) ANOTHER HIM",
                               "(Ach. 39) Alpha amplification II",
                               "(Ach. 40) Replicanti",
                               "(Ach. 41) Triple A",
                               "(Ach. 42) A truly useful one",
                               "(Ach. 43) The second quarter",
                               "(Ach. 44) The third 90 degrees",
                               "(Ach. 45) Full Circle",
                               "(Ach. 46) Far from enough",
                               "(Ach. 47) Not yet effective",
                               "(Ach. 48) Just Enough AUP",
                               "(Ach. 49) Rearrangement",
                               "(Ach. 50) You forgot to Prestige",
                               "(Ach. 51) Hypodrops",
                               "(Ach. 52) Halfway to there"
                              };
const string ach_desc[1000] = {"",
                               "Purchase an A1.",
                               "Purchase an I1.",
                               "Purchase an A2.",
                               "Purchase an I2.",
                               "Purchase an A3.",
                               "Prestige once.",
                               "Purchase an I3.",
                               "Purchase PU1 (Under Prestige tab).",
                               "Purchase an Amp. Aero.",
                               "Purchase an Int. Aero.",
                               "Purchase an A4.",
                               "Purchase an I4.",
                               "Purchase PU2.",
                               "Purchase an A5.",
                               "Purchase an I5.",
                               "Purchase PU3.",
                               "Purchase an A6.",
                               "Purchase an I6.",
                               "Purchase an A7.",
                               "Purchase an I7.",
                               "Purchase PU4.",
                               "Purchase an A8.",
                               "Purchase an I8.",
                               "Purchase 9 Amp. Aero.",
                               "Purchase 5 Int. Aero.",
                               "Reach Alpha.",
                               "Use an Alpha Upgrade Point.",
                               "Get the second Alpha milestone.",
                               "Get the third Alpha milestone.",
                               "Get the fourth Alpha milestone.",
                               "Purchase BAU1.",
                               "Purchase BAU2.",
                               "Get the fifth milestone.",
                               "Get the sixth Alpha milestone.",
                               "Get the seventh Alpha milestone.",
                               "Purchase BAU3.",
                               "Purchase BAU4.",
                               "Purchase BAU5.",
                               "Purchase BAU6.",
                               "Reach 1e10 Alpha matter.",
                               //41-
                               "Purchase Alpha Amp. Aero.",
                               "Purchase BAU7.",
                               "Purchase BAU8.",
                               "Purchase BAU9.",
                               "Purchase BAU10.",
                               "Reach e1.00e10 matter.",
                               "Have PIM less than 0.",
                               "Use 1e9 AUP.",
                               "Reach SL 8.",
                               "Reach 1e10000000 matter with\nGIM equal to 10.",
                               "Reach SL 10.",
                               "Have 5 Tier 2 Drops."
                              };
const string ach_reward[1000] = {"",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "Reward: x2 to AP and Alpha.",
                                 "Reward: x2 to AP.",
                                 "",
                                 "Reward: Add an Prestige effect same\nas PU2.",
                                 "Reward: x1.01 to ALL Intensifiers.", //Bookmark: 33 here
                                 "Reward: x1.01 to ALL Intensifiers.",
                                 "Reward: x1.01 to ALL Intensifiers.",
                                 "Reward: x1.01 to ALL Intensifiers.",
                                 "Reward: x2 to AP.\nImprove Ach. formula.",
                                 "",
                                 "",
                                 "Reward: Unlock Alpha Simulator.",
                                 //41-
                                 "",
                                 "Reward: +50% Value to Drops,\n+50% Init. speed to Drops.",
                                 "Reward: Mult. SP by (SL+1).",
                                 "Reward: Boost gravity based on SL.",
                                 "Reward: Add more balls on the board.",
                                 "Reward: x2 to Drops' gravity.",
                                 "Reward: Add 5 free levels to \nGlobal upgrades.",
                                 "Reward: Square Alpha Sim. bonus.",
                                 "Reward: Add even more balls on the\nboard.",
                                 "Reward: Mult. Value of Drops by\n(SL+1).",
                                 "Reward: Mult. SP by log10(AP).",
                                 "Reward: Mult. SP by log10(Am)."
								};

const string story_text[100] = {
	"",
	// Story 1
	"...So they reborn.\n\
    They found themselves in a new world,\n\
    and the first thing in front of them is an Amplifier.\n\
    So they bought it, then intensified.\n\
    They start to accumulate matter.\n\
    They conquered mysteries.\n\
    ...and finally meet their first threshold.",
	// Story 2
	"...They walked past the first threshold.\n\
    Then the second.\n\
    Then a third.\n\
	They did not remember crossing.\n\
    The world looked familiar, but slightly wrong.\n\
    The numbers came easier now.\n\
    So They did not stop to wonder why.\n\
    ...They should have.",
	// Story 3
	"...They found a crack in the world.\n\
    Through it, something unfamiliar.\n\
    Not matter, nor energy, nor anything They had a name for.\n\
    A pale, silent thing.\n\
    It did not fight Them.\n\
    It did not fear Them.\n\
    ...It was simply there.\n\
    And They, in Their hunger, took it.",
	// Story 4
	"...The pale thing multiplied.\n\
    It spread through Their world like warmth in winter.\n\
    Everything flourished.\n\
    Their gains doubled, tripled, beyond counting.\n\
    It whispered nothing, yet They understood:\n\
    this was a gift.\n\
    ...They did not ask who was giving.",
	// Story 5
	"...Am touched the edge of the world.\n\
    For one suspended breath, They saw it:\n\
    a thin membrane, holding everything back.\n\
    Beyond it, something endless.\n\
    Something patient.\n\
    Something that watched Them\n\
    the way a gardener watches a growing seed.\n\
    ...And for the first time, They wondered\n\
    whether They were the gardener,\n\
    or the seed.",
    // Story 6
    "...and They decide to simulate a small\n\
	world to gain more power.\n\
	The drops fall. The spheres shatter.\n\
	It feels... ephemeral."
};

const string story_hint[100] = {
	"",
	"...until They did it the second time.",
	"...but They did not count the crossings.",
	"...if only They had looked beneath.",
	"...the gift never asked for thanks.",
	"...until the very edge of the world.",
	"...when They started a new world."
};

const string story_reward[100] = {
	"",
	"Reward: Mult. to Amplifiers based on time spent in this Alpha.",
	"Reward: AP gain x(1 + 0.1 x Alpha).",
	"Reward: AA and AI Amplifiers gain +10% base multiplier.",
	"Reward: Am production x2.",
	"Reward: GIM reduction x2.",
	"Reward: A fading sense of accomplishment."
};
#ifdef _WIN32
#include "imgui.h"
struct Notification;
void PushNotification(const std::string& title,
                      const std::string& message,
                      const ImVec4& accent);
#endif

static Number level_inc(const Number& lv) {
	if (lv.r.layer == 0 && lv.r.val < 1e15L) {
		return lv + ONE;
	}
	Number delta = lv * _to_Real(1e-15L);
	if (delta < ONE) delta = ONE;
	return lv + delta;
}

// Temporary variable for storing Original values and Softcapped values.
// Where, softcap[x][0] = Original value, softcap[x][1] = Softcapped Value.
Number softcap[2000][2];

class player_ {
	public:
		player_() : upgrade(1000), options(1000), ach(1000, false), story(1000, false) {}

		std::vector<upg_> upgrade;

		// Options list:
		// 0: Is 0 if first Time Flux mode is selected.
		// 1: First Time Flux Mode (Spend xx:xx:xx)
		// 2: Second Time Flux Mode (Spend x%)
		// 3. Savefile name.
		// 4. The BGM index, 1(initial) = "The Beginning.mp3"
		// 5. The BGM volume, from 0 to 100, initially 50
		std::vector<opt_> options;
		pre_alpha_ pre_alpha;
		alpha_ alpha;
		beta_ beta;
		Number fastest_ap_per_min = 0;
		Number fastest_bp_per_min = 0;
		std::vector<bool> ach;   // Whether Ach. x is completed.
		std::vector<bool> story; // Whether Story x is found.
		Number GIM = TEN;
		Number PIM = TEN;
		Number PF  = TEN;
		long double last_tick = 0; // Used to calculate time passage.
		long double time_flux = 0; // How much time flux you have.

		Number rate_Number = 0;
		Number rate_Number_Am = 0;
		Number rate_Number_Bm = 0;

		// Which tab you are in and how many tabs you have unlocked.
		// Tab 1: Production.   (Initial)
		// Tab 2: Options.      (Initial)
		// Tab 3: Achievements. (Initial)
		// Tab 4: Prestige.     (Unlocks after Prestige)
		// Tab 5: Alpha.        (Unlocks after Alpha)
		// Tab 6: Beta.         (Unlocks after Beta)
		int currenttab = 1;
		int unlockedtabs = 3;

		// Self-explanatory.
		bool unlocked_prestige = false;
		bool unlocked_alpha = false;
		bool break_alpha = false;
		bool unlocked_beta = false;

		// ---------------- Ach. & Story related functions.

		// Returns whether Ach. x is completed.
		[[nodiscard]] bool _get_ach(int id) {
			return this->ach[id];
		}

		// Returns how many Ach.s are completed.
		// Used to calculate overall Ach. reward.
		[[nodiscard]] Number _get_ach_done() {
			Number cnt = ZERO;
			for(int i = 1; i <= 52; i ++) {
				cnt += _get_ach(i) ? ONE : ZERO;
			}
			return cnt;
		}

		// Returns whether Story x is found.
		[[nodiscard]] bool _get_story(int id) {
			return this->story[id];
		}

		// Returns how many stories are found.
		[[nodiscard]] Number _get_story_done() {
			Number cnt = ZERO;
			for(int i = 1; i <= 50; i ++) {
				cnt += _get_story(i) ? ONE : ZERO;
			}
			return cnt;
		}

		// Unlocks Ach. x.
		void _unlock(int id) {
			if(_get_ach(id)) return;
			else {
				this->ach[id] = true;

#ifdef _WIN32
				if (g_use_gui) {
					std::string title = std::string("Achievement Unlocked");
					std::string body  = ach_name[id];
					if (body.size() >= 8 && body.substr(0, 8) == "(Ach. ") {
						// Strip the "(Ach. N) " prefix for cleanliness
						size_t close = body.find(')');
						if (close != std::string::npos && close + 2 < body.size())
							body = body.substr(close + 2);
					}
					if (!ach_desc[id].empty()) body += "\n" + ach_desc[id];
					if (!ach_reward[id].empty()) body += "\n" + ach_reward[id];
					PushNotification(title, body, ImVec4(1.0f, 0.85f, 0.35f, 1.0f)); // gold
				}
#endif

			}
		}

		// Unlock certain ach. if certain requirements are met.
		void _try_unlock(int id) {
			if(_get_ach(id)) return;

			//Requirements in previous definition.
			if(id == 1 && (!this->upgrade[0].level.is_zero())) {
				_unlock(id);
			}
			if(id == 2 && (!this->upgrade[8].level.is_zero())) {
				_unlock(id);
			}
			if(id == 3 && (!this->upgrade[1].level.is_zero())) {
				_unlock(id);
			}
			if(id == 4 && (!this->upgrade[9].level.is_zero())) {
				_unlock(id);
			}
			if(id == 5 && (!this->upgrade[2].level.is_zero())) {
				_unlock(id);
			}
			if(id == 6 && (this->unlockedtabs >= 4)) {
				_unlock(id);
			}
			if(id == 7 && (!this->upgrade[10].level.is_zero())) {
				_unlock(id);
			}
			if(id == 8 && (!this->upgrade[16].level.is_zero())) {
				_unlock(id);
			}
			if(id == 9 && (!this->upgrade[20].level.is_zero())) {
				_unlock(id);
			}
			if(id == 10 && (!this->upgrade[21].level.is_zero())) {
				_unlock(id);
			}
			if(id == 11 && (!this->upgrade[3].level.is_zero())) {
				_unlock(id);
			}
			if(id == 12 && (!this->upgrade[11].level.is_zero())) {
				_unlock(id);
			}
			if(id == 13 && (!this->upgrade[17].level.is_zero())) {
				_unlock(id);
			}
			if(id == 14 && (!this->upgrade[4].level.is_zero())) {
				_unlock(id);
			}
			if(id == 15 && (!this->upgrade[12].level.is_zero())) {
				_unlock(id);
			}
			if(id == 16 && (!this->upgrade[18].level.is_zero())) {
				_unlock(id);
			}
			if(id == 17 && (!this->upgrade[5].level.is_zero())) {
				_unlock(id);
			}
			if(id == 18 && (!this->upgrade[13].level.is_zero())) {
				_unlock(id);
			}
			if(id == 19 && (!this->upgrade[6].level.is_zero())) {
				_unlock(id);
			}
			if(id == 20 && (!this->upgrade[14].level.is_zero())) {
				_unlock(id);
			}
			if(id == 21 && (!this->upgrade[19].level.is_zero())) {
				_unlock(id);
			}
			if(id == 22 && (!this->upgrade[7].level.is_zero())) {
				_unlock(id);
			}
			if(id == 23 && (!this->upgrade[15].level.is_zero())) {
				_unlock(id);
			}
			if(id == 24 && (this->upgrade[20].level >= 9)) {
				_unlock(id);
			}
			if(id == 25 && (this->upgrade[21].level >= 5)) {
				_unlock(id);
			}

			//Alpha part.
			if(id == 26 && (this->unlocked_alpha)) {
				_unlock(id);
			}
			if(id == 27 && (_get_AUP_bought() > ZERO)) {
				_unlock(id);
			}
			if(id == 28 && (_get_AM(1))) {
				_unlock(id);
			}
			if(id == 29 && (_get_AM(2))) {
				_unlock(id);
			}
			if(id == 30 && (_get_AM(3))) {
				_unlock(id);
			}
			if(id == 31 && (!this->upgrade[30].level.is_zero())) {
				_unlock(id);
			}
			if(id == 32 && (!this->upgrade[31].level.is_zero())) {
				_unlock(id);
			}
			if(id == 33 && (_get_AM(4))) {
				_unlock(id);
			}
			if(id == 34 && (_get_AM(5))) {
				_unlock(id);
			}
			if(id == 35 && (_get_AM(6))) {
				_unlock(id);
			}
			if(id == 36 && (!this->upgrade[32].level.is_zero())) {
				_unlock(id);
			}
			if(id == 37 && (!this->upgrade[33].level.is_zero())) {
				_unlock(id);
			}
			if(id == 38 && (!this->upgrade[34].level.is_zero())) {
				_unlock(id);
			}
			if(id == 39 && (!this->upgrade[35].level.is_zero())) {
				_unlock(id);
			}
			if(id == 40 && (this->alpha.score >= 1e10)) {
				_unlock(id);
				this->alpha.unlocked_simulator = true;
			}

			if(id == 41 && (!this->upgrade[56].level.is_zero())) {
				_unlock(id);
			}
			if(id == 42 && (!this->upgrade[36].level.is_zero())) {
				_unlock(id);
			}
			if(id == 43 && (!this->upgrade[37].level.is_zero())) {
				_unlock(id);
			}
			if(id == 44 && (!this->upgrade[38].level.is_zero())) {
				_unlock(id);
			}
			if(id == 45 && (!this->upgrade[39].level.is_zero())) {
				_unlock(id);
			}
			if(id == 46 && (this->pre_alpha.score >= _TEN.pow(1e10))) {
				_unlock(id);
			}
			if(id == 47 && (this->PIM <= 0)) {
				_unlock(id);
			}
			if(id == 48 && (_get_AUP_bought() >= 1e9)) {
				_unlock(id);
			}
			if(id == 49 && this->alpha.simulator.level >= 8) {
				_unlock(id);
			}
			if(id == 50 && this->pre_alpha.score >= TEN.pow(1e7) && this->_get_GIM() == _TEN) {
				_unlock(id);
			}
			if(id == 51 && this->alpha.simulator.level >= 10) {
				_unlock(id);
			}
			if(id == 52 && this->alpha.simulator.drops[2] >= 5) {
				_unlock(id);
			}
		}

		// Unlocks Story x if certain requirements are met.
		void _unlock_story(int id) {
			if(_get_story(id)) return;
			else {
				this->story[id] = true;
#ifdef _WIN32
				if (g_use_gui) {
					std::string title = "Story " + std::to_string(id) + " Unlocked";
					std::string body  = story_reward[id].empty() ? story_hint[id] : story_reward[id];
					PushNotification(title, body, ImVec4(0.55f, 0.85f, 1.0f, 1.0f)); // cyan
				}
#endif
			}
		}

		// Unlock certain Story if certain requirements are met.
		void _try_unlock_story(int id) {
			if(id == 1 && (this->unlocked_alpha && !this->upgrade[7].level.is_zero())) {
				_unlock_story(id);
			}
			// Story 2: reached Alpha 3 times.
			if(id == 2 && (this->alpha.alpha >= 3)) {
				_unlock_story(id);
			}
			// Story 3: unlocked the Alpha Amplifier / Intensifier system.
			if(id == 3 && (this->upgrade[34].level.is_one())) {
				_unlock_story(id);
			}
			// Story 4: Am reached 1e10.
			if(id == 4 && (this->alpha.score >= 1e10)) {
				_unlock_story(id);
			}
			// Story 5: Am reached the world's edge.
			if(id == 5 && (this->alpha.score >= inf)) {
				_unlock_story(id);
			}
			// Story 6: Unlock Alpha Simulator.
			if(id == 6 && (this->alpha.unlocked_simulator)) {
				_unlock_story(id);
			}
		}

		// Unlock certain Ach. and Story if certain requirements are met.
		void _try_unlock_all() {
			for(int i = 1; i <= 52; i ++) _try_unlock(i);
			for(int i = 1; i <= 50; i ++) _try_unlock_story(i);
		}

		// Returns Ach. effect.
		// Achievements give a multiplier to:
		// id = 1: Amplifiers, x(1+0.001x)
		[[nodiscard]] Number _get_ach_effect(int id) {
			if(id == 1) {
				if(!this->ach[37]) return ONE + 0.001 * _get_ach_done();
				else return ONE + 0.005 * _get_ach_done();
			}
			return ONE;
		}

		// Returns effect of Story x.
		// id = 1: Amplifiers, x(1+0.001s), s = game-time seconds passed.
		// id = 2: AP gain multiplier, x(1 + 0.1 * Alpha).
		// id = 3: AA/AI base multiplier, x1.1.
		// id = 4: Am production, x2.
		// id = 5: GIM reduction, x2.
		[[nodiscard]] Number _get_story_effect(int id) {
			if(id == 1 && this->story[id]) {
				Number tm = (this->alpha.time) / 1000.0;
				return tm + ONE;
			}
			if(id == 2 && this->story[id]) {
				return ONE + 0.1 * this->alpha.alpha;
			}
			if(id == 3 && this->story[id]) {
				return 1.1;
			}
			if(id == 4 && this->story[id]) {
				return 2;
			}
			if(id == 5 && this->story[id]) {
				return 2;
			}
			return ONE;
		}

		// --------------------------- Pre-prestige related items.

		// Returns current Prestige Factor, or PF.
		// It is used in the GIM -> Mult formula (10 - logPF(log10(x)) / 2).
		// It starts off at 10 and decreases as PIM decreases, which is decreased by PU3.
		// Less PF is better, but it should NEVER be less than 1.
		// ** PU3: Decrease PIM at 10% rate of GIM reduction.
		[[nodiscard]] Number _get_PF() const {
			Number init = TEN;
			Number pim_ld = PIM;
			Number exp = pim_ld / TEN;
			if (pim_ld >= ZERO) init = 8 * Number(1.25).pow(exp);
			else if (pim_ld < ZERO && PIM >= -100) init = 8 * Number(1.25).pow(exp / TEN);
			else if (pim_ld < -100) init = ONE + 5.521852 * Number(1.25).pow(exp / TEN / TEN);
			return init;
		}
		void _update_PF() {
			this->PF = _get_PF();
			PF = this->PF;
		}

		// Returns current Prestige Instability Magnitude, or PIM.
		// It is used to decrease PF, which is used to improve GIM -> Mult formula.
		// It starts off as 10 and is decreased by PU3.
		// Less is better.
		[[nodiscard]] Number _get_PIM() {
			Number init = TEN;
			Number reduction = ZERO;
			if(this->upgrade[18].level.is_one()) reduction = _get_PU3_effect();
			return init - reduction;
		}
		void _update_PIM() {
			this->PIM = _get_PIM();
			PIM = this->PIM;
		}

		// Returns current Global Instability Magnitude, or GIM.
		// It is the major instability magnitude (in the beginning), though later it may change into a buff (GIM < 0).
		// It is decreased by prestige and more matter AT prestige will decrease it, which increases Mult empowerment.
		Number _get_GIM() {
			Number init = TEN;
			this->PF = _get_PF();
			Number highest_ld = this->pre_alpha.highest;
			if (((highest_ld - ONE) < 1e-9) && ((ONE - highest_ld) < 1e-9)
			        || ((highest_ld) < 1e-9) && ((highest_ld) > -1e-9)) {
				return init;
			}
			Number tmp = (this->pre_alpha.highest + 1);
			tmp = tmp.log(TEN);
			tmp = tmp + ONE;
			tmp = tmp.log(this->PF);
			Number gim_red = _get_AU_effect(2) * _get_Am_effect(1);
			if (_get_story(5)) gim_red = gim_red * _get_story_effect(5);
			Number reduction = tmp / 2 * gim_red;
			tmp = reduction - init;
			if(tmp > TEN) tmp = tmp.pow(0.9) * _TEN.pow(0.1);
			tmp = ZERO - tmp;
			return tmp;
		}

		// Returns current GIM IF you prestige at this point, used for display.
		Number _get_GIM(Number _score) {
			Number init = TEN;
			this->PF = _get_PF();
			Number highest_ld = _score;
			if (((highest_ld - ONE) < 1e-9) && ((ONE - highest_ld) < 1e-9)
			        || ((highest_ld) < 1e-9) && ((highest_ld) > -1e-9)) {
				return init;
			}
			Number tmp = (_score + 1);
			tmp = tmp.log(TEN);
			tmp = tmp + ONE;
			tmp = tmp.log(this->PF);
			Number gim_red = _get_AU_effect(2) * _get_Am_effect(1);
			if (_get_story(5)) gim_red = gim_red * _get_story_effect(5);
			Number reduction = tmp / 2 * gim_red;
			tmp = reduction - init;
			if(tmp > TEN) tmp = tmp.pow(0.9) * _TEN.pow(0.1);
			tmp = ZERO - tmp;
			return tmp;
		}

		// Returns how much PIM decreases PF.
		Number _get_PIM_effect() {
			return TEN - _get_PF();
		}

		// Returns GIM -> M^(x).
		[[nodiscard]] Number _get_GIM_effect() {
			Number gim = _get_GIM();
			if ((gim - TEN) < 1e-9 && (TEN - gim) < 1e-9) {
				return 0.5;
			}

			Number prod = 11;
			prod = prod - gim;
			prod = prod.log(121);
			prod = (ONE + ONE) * prod * prod + 0.5;
			if(prod > 1.3225) prod = Real(1.3225).pow(0.75) * prod.pow(0.25);
			return prod;
		}
		void _update_GIM() {
			this->GIM = _get_GIM();
			GIM = this->GIM;
		}

		// ---------------------------- Prestige related items.

		// The current PU3 effect. Work even if PU3 is not bought.
		[[nodiscard]] Number _get_PU3_effect() {
			Number init = TEN;
			if (this->pre_alpha.highest.is_zero() || this->pre_alpha.highest.is_one()) {
				return ZERO;
			}
			Number pf = this->PF;
			Number tmp = (this->pre_alpha.highest + 1);
			tmp = tmp.log(TEN);
			tmp = tmp + ONE;
			tmp = tmp.log(pf);
			Number gim_red = _get_AU_effect(2) * _get_Am_effect(1);
			if (_get_story(5)) gim_red = gim_red * _get_story_effect(5);
			Number reduction = tmp / 2 * gim_red;
			if(reduction > TEN) reduction = reduction.pow(0.9) * TEN.pow(0.1);
			reduction = reduction / TEN;
			return reduction;
		}


		// Returns the effect of PU2, which gives a multiplier to Amp. preduction that applies BEFORE Int. effect.
		// **PU2: Give a small multiplier based on matter to Mx/s which applies before Intensifier effect.
		[[nodiscard]] Number _get_PU2_effect() {
			if(this->pre_alpha.score <= FIVE.pow(FIVE)) return ONE;
			Number init = this->pre_alpha.score.log(FIVE).log(FIVE);
			init = ONE + (init - ONE) / FIVE;
			return init;
		}

		// Used in an Ach. reward.(Ach. 33: Get the 5th Alpha Milestone.)
		[[nodiscard]] Number _get_PU2_effect(Number s) {
			if(s <= FIVE.pow(FIVE)) return ONE;
			Number init = s.log(FIVE).log(FIVE);
			init = ONE + (init - ONE) / FIVE;
			return init;
		}

		// Get Aeromagnifier effects.
		[[nodiscard]] Number _get_aero_effect_a() {
			Number a = (ONE + 0.15 * this->upgrade[20].level) * _get_AU_effect(0);
			if(!this->upgrade[19].level.is_zero()) a = (ONE + 0.2 * this->upgrade[20].level) * _get_AU_effect(0);
			return a;
		}
		[[nodiscard]] Number _get_aero_effect_i() {
			Number a = (ONE + 0.075 * this->upgrade[21].level) * _get_AU_effect(1);
			if(!this->upgrade[19].level.is_zero()) a = (ONE + 0.1 * this->upgrade[20].level) * _get_AU_effect(0);
			if(_get_ach(33)) a = a * 1.01;
			if(_get_ach(34)) a = a * 1.01;
			if(_get_ach(35)) a = a * 1.01;
			if(_get_ach(36)) a = a * 1.01;
			return a;
		}

		// ----------------------------------- Alpha related items.

		[[nodiscard]] Number _get_alpha_aero_effect_a() {
			Number a = (ONE + 0.15 * this->upgrade[56].level);
			return a;
		}
		[[nodiscard]] Number _get_alpha_aero_effect_i() {
			Number a = ONE + 0.075 * this->upgrade[57].level;
			if(_get_ach(33)) a = a * 1.01;
			if(_get_ach(34)) a = a * 1.01;
			if(_get_ach(35)) a = a * 1.01;
			if(_get_ach(36)) a = a * 1.01;
			if(_get_SM(4)) a = a * _get_ach_effect(1);
			return a;
		}

		// Returns whether Alpha Milestone (id+1) is completed.
		bool _get_AM(int id) {
			if (this->alpha.alpha >= am[id]) return true;
			if (_get_BM(id)) return true;
			Number diff = am[id] - this->alpha.alpha;
			if (diff.r.layer == 0.0L && diff.r.sign && diff.r.neg > 0 &&
			        diff.r.val < 0.01L) {
				return true;
			}
			return false;
		}

		// Returns how many AP you can get with current matter. (this->alpha.highest)
		[[nodiscard]] Number _get_AP() {
			Number a = ZERO;
			if(!this->break_alpha || this->pre_alpha.score < inf) a = ONE;
			else a = this->pre_alpha.score.log(inf);
			a = a * Number(4).pow(this->upgrade[30].level);
			if(_get_ach(30)) a = a * 2;
			if(_get_ach(31)) a = a * 2;
			if(_get_story(2)) a = a * _get_story_effect(2);
			a = a * this->_get_BAU10_effect();
			return a;
		}

		[[nodiscard]] Number _get_AP(Number s) {
			Number a = ZERO;
			Number b = s.normalize();
			if(!this->break_alpha || b < inf) a = ONE;
			else a = b.log(inf);
			a = a * Number(4).pow(this->upgrade[30].level);
			if(_get_ach(30)) a = a * 2;
			if(_get_ach(31)) a = a * 2;
			if(_get_story(2)) a = a * _get_story_effect(2);
			a = a * this->_get_BAU10_effect();
			return a;
		}

		// Returns how many alpha(s) you can get if you (perform) alpha now.
		[[nodiscard]] Number _get_alpha() {
			Number a = ONE;
			if(_get_ach(30)) a = a * 2;

			// ** BAU3 : Multiply Alpha gain based on matter and level.
			if (this->upgrade[32].level > ZERO) {
				Number s = this->pre_alpha.score;
				if (s > ONE) {
					Number log_s = s.log(TEN);
					if (log_s > ZERO) {
						Number x = this->upgrade[32].level;
						Number exponent = 0.75 - 0.75 / Number(1.1).pow(x);
						if (exponent > 0.75) exponent = 0.75;
						Number mul = log_s.pow(exponent);
						a = a * mul;
					}
				}
			}
			if (this->unlocked_beta) a = a * _get_Bm_effect(4);
			if (a < ONE) return ONE;
			return floor(a) + 0.0000001;
		}

		// Returns the cost of the next AUP purchase of a given id.
		// id = 26: with matter
		// id = 27: with Amp. Aero
		// id = 28: with Int. Aero
		// id = 29: with AP
		[[nodiscard]] Number _get_AUP_cost(int id) {
			if (id == 26) {
				Number s = 1e100;
				Number level = this->upgrade[26].level;
				Number exp;
				if (level <= 10) {
					exp = level + ONE;
				} else {
					Number extra = level - 10;
					exp = (level + ONE) + extra * extra;
				}

				s = _get_third_scale(s, s.pow(exp));

				return s.pow(exp);
			}
			if (id == 27) {
				Number level = this->upgrade[27].level;
				Number cost = 5 + 5 * level;
				if (level > 10) {
					Number extra = level - 10;
					cost = cost + extra * extra;
				}
				return cost;
			}
			if (id == 28) {
				Number level = this->upgrade[28].level;
				Number cost = 4 + 3 * level;
				if (level > 10) {
					Number extra = level - 10;
					cost = cost + extra * extra;
				}
				return cost;
			}
			if (id == 29) {
				Number s = 2;
				Number level = this->upgrade[29].level;
				Number exp;
				if (level <= 10) {
					exp = level;
				} else {
					Number extra = level - 10;
					exp = level + extra * extra;
				}
				return s.pow(exp);
			}
			return ZERO;
		}

		// The resource each AUP type consumes, for UI display.
		[[nodiscard]] const char* _get_AUP_unit(int id) {
			if (id == 26) return " matter";
			if (id == 27) return " Amp. Aero";
			if (id == 28) return " Int. Aero";
			if (id == 29) return " AP";
			return "";
		}

		// Returns how many Alpha Upgrade Points (AUP) you have bought.
		[[nodiscard]] Number _get_AUP_bought() {
			return this->upgrade[22].level+this->upgrade[23].level+this->upgrade[24].level+this->upgrade[25].level;
		}

		// Returns how many AUP is used.
		[[nodiscard]] Number _get_AUP() {
			return this->upgrade[26].level+this->upgrade[27].level+this->upgrade[28].level+this->upgrade[29].level;
		}

		// Returns how many AUP is avaliable.
		[[nodiscard]] Number _get_AUP_avaliable() {
			return _get_AUP() - _get_AUP_bought();
		}


		// Returns the numerical effect of Alpha Upgrades. (ID 22-25)
		// Input is 0-3, not 1-4 or 22-25.
		[[nodiscard]] Number _get_AU_effect(int id) {
			if(id == 0) return Number(1.1).pow(this->upgrade[id + 22].level);
			if(id == 1) {
				Number level = this->upgrade[23].level;
				Number base  = 1.02;
				const Number THRESHOLD = 25;

				if (level <= THRESHOLD) {
					return base.pow(level);
				}

				Number extra      = level - THRESHOLD;
				Number log2_term  = (extra + ONE).log(2);

				return base.pow(THRESHOLD + log2_term);
			}
			if(id == 2) {
				Number base =  ONE + 0.05 * this->upgrade[id + 22].level;
				if(base >= 2) base = (8 * base).pow(0.25);
				return base;
			}
			if(id == 3) {
				Number base = Number(0.99).pow(this->upgrade[id + 22].level);
				Number x = this->upgrade[id + 22].level;
				if(base <= 0.01) base = (Number(0.01).pow(ONE - ONE / x.pow(0.5)) * base.pow(ONE / x.pow(0.5)));
				return base;
			}
			return ONE;
		}

		// Alpha Amplifier / Alpha Intensifier related methods

		// Returns BAU effect.
		// id = 0: x4^x to AP gained.
		// id = 1: Mult. to Amplifiers based on this BAU's level and Alpha.
		Number _get_BAU_effect(int id) {
			if(id == 0) {
				return Number(4).pow(this->upgrade[id + 30].level);
			}
			if(id == 1) {
				if(this->upgrade[id + 30].level.is_zero()) return ONE;
				Number base = ONE + 0.1 * this->alpha.alpha;
				return base.pow(ONE + 0.1 * this->upgrade[id + 30].level);
			}
			return ONE;
		}

		// BAU8: x (log10(AP))^0.5 to Am generation
		[[nodiscard]] Number _get_BAU8_effect() {
			if (!this->upgrade[37].level.is_one()) return ONE;
			Number ap = this->alpha.point;
			if (ap <= ONE) return ONE;
			Number log10_ap = ap.log(TEN);
			if (log10_ap <= ZERO) return ONE;
			if(!_get_SM(5)) return log10_ap.pow(0.5);
			else return log10_ap.pow(2.5);
		}

		// BAU9: -log10(AP)/100 to AIM
		[[nodiscard]] Number _get_BAU9_effect() {
			if (!this->upgrade[38].level.is_one()) return ZERO;
			Number ap = this->alpha.point;
			if (ap <= ONE) return ZERO;
			Number log10_ap = ap.log(TEN);
			if (log10_ap <= ZERO) return ZERO;
			if(!_get_SM(6)) return log10_ap / 100;
			else return log10_ap / 100 * 3;
		}

		// BAU10: x log10(Am) * level to AP gain
		[[nodiscard]] Number _get_BAU10_effect() {
			if (this->upgrade[39].level <= ZERO) return ONE;
			Number am = this->alpha.am_best;
			if (am <= ONE) return ONE;
			Number log10_am = am.log(TEN);
			if (log10_am <= ONE) return ONE;
			return log10_am * this->upgrade[39].level;
		}

		[[nodiscard]] Number _get_AIM() {
			Number base = TEN;
			if (!this->upgrade[38].level.is_zero()) base = base - this->_get_BAU9_effect();
			return base;
		}
		void _update_AIM() {
			this->alpha.AIM = _get_AIM();
		}

		[[nodiscard]] Number _get_AIM_effect() {
			_update_AIM();
			if (this->alpha.AIM == TEN) return 0.25;
			Number prod = 11;
			prod = prod - this->alpha.AIM;
			prod = prod.log(121);
			prod = ((ONE + ONE) * prod * prod + 0.5) / 2;
			return prod.normalize();
		}

		// Am's 4 effects, given id = 1~4.
		//   id = 1: Mult. to GIM reduction.
		//   id = 2: Mult. to PIM reduction.
		//   id = 3: Mult. to A1-A8.
		//   id = 4: Mult. to I1-I8.
		[[nodiscard]] Number _get_Am_effect(int id) {
			if (id < 1 || id > 4) return ONE;
			Number base = this->alpha.am_best;
			if (base <= ONE) return ONE;

			Number log10_base = base.log(TEN);
			if (log10_base <= ZERO) return ONE;


			if (id == 1) return (ONE + log10_base * 0.05).pow(_get_Bm_effect(3));
			if (id == 2) return (ONE + log10_base * 0.05).pow(_get_Bm_effect(3));
			if (id == 3) return (ONE + log10_base * 0.1).pow(_get_Bm_effect(3));
			if (id == 4) return (ONE + log10_base * 0.01).pow(_get_Bm_effect(3));
			return ONE;
		}

		// Returns whether Simulation Milestone (id+1) is completed.
		[[nodiscard]] bool _get_SM(int id) {
			if (id < 0 || id >= 8) return false;
			return this->alpha.simulator.level >= sm[id];
		}

		// Returns the effect of Simulation Milestone.
		[[nodiscard]] Number _get_SM_effect(int id) {
			if(id == 1) return _get_story_effect(1);
			if(id == 2) return this->alpha.score.log(TEN);
			if(id == 3) return this->alpha.point.log(TEN);
			if(id == 4) return _get_ach_effect(1);
			if(id == 5) return _get_BAU8_effect();
			if(id == 6) return _get_BAU9_effect();
			if(id == 7) {
				if(this->pre_alpha.score < TEN.pow(TEN)) return ONE;
				else return this->pre_alpha.score.log(TEN).log(TEN);
			}
			if(id == 8) {
				return ONE;
			}
		}

		// Returns the effect of Simulation Points.
		[[nodiscard]] Number _get_SP_effect(int id) {
			Number log10_sp = this->alpha.simulator.score.log(TEN);
			if (id == 1){
				Number a = (log10_sp >= ONE) ? log10_sp : ONE;  // AP multiplier
				if(_get_ach(48)) a = a * a;
				return a;
			}
			if (id == 2){
				Number a = (log10_sp >= ONE) ? log10_sp : ONE;  // Am generation multiplier
				if(_get_ach(48)) a = a * a;
				return a;
			}
			if (id == 3){
				Number a = (log10_sp >= ONE) ? log10_sp : ONE;  // SP multiplier
				a = a * a;
				if(_get_ach(48)) a = a * a;
				return a;
			}
			return ONE;
		}
		
		// ------------------------------ Beta related items.

		[[nodiscard]] Number _get_beta_aero_effect_a() {
			Number a = ONE + 0.15 * this->upgrade[74].level;
			return a;
		}
		[[nodiscard]] Number _get_beta_aero_effect_i() {
			Number a = ONE + 0.075 * this->upgrade[75].level;
			return a;
		}

		// Returns whether Beta Milestone (id+1) is completed.
		bool _get_BM(int id) {
			if (this->beta.beta >= bm[id]) return true;

			Number diff = bm[id] - this->beta.beta;
			if (diff.r.layer == 0.0L && diff.r.sign && diff.r.neg > 0 &&
			        diff.r.val < 0.01L) {
				return true;
			}
			return false;
		}

		// Returns how much BP is gained on Beta.
		[[nodiscard]] Number _get_BP() {
			if(!this->unlocked_beta) return ONE;
			return ONE;
		}

		[[nodiscard]] Number _get_BP(Number s) {
			if(!this->unlocked_beta) return ONE;
			return ONE;
		}

		[[nodiscard]] Number _get_beta() {
			if(!this->unlocked_beta) return ONE;
			Number a = ONE;
			a = a * _get_Bm_effect(5);
			return a;
		}

		[[nodiscard]] Number _get_BIM() {
			Number base = TEN;
			// Placeholder.

			return base;
		}
		void _update_BIM() {
			this->beta.BIM = _get_BIM();
		}

		[[nodiscard]] Number _get_BIM_effect() {
			_update_BIM();
			if (this->beta.BIM == TEN) return 0.1;
			Number prod = 11;
			prod = prod - this->alpha.AIM;
			prod = prod.log(121);
			prod = ((ONE + ONE) * prod * prod + 0.5) / 5;
			return prod.normalize();
		}

		// Returns the Beta Matter (Bm) effects.
		// id = 1: Narrow gap between BDN and 1 by ^(1+sqrt(logBm)/100).
		// id = 2: Narrow gap between BDF and 1 by ^(1+sqrt(logBm)/100).
		// id = 3: Empower Am effects by ^(1+sqrt(logBm)/100).
		// id = 4: Multiply Alpha gain by logBm.
		// id = 5: Multiply Beta gain by double-log of Bm.
		[[nodiscard]] Number _get_Bm_effect(int id) const {
			if (id < 1 || id > 5) return ONE;
			Number base = ONE;
			if(this->beta.score > TEN) base = ONE + (this->beta.score).pow(0.5) / 100;

			if (id >= 1 && id <= 3) return base;
			if (id == 4) {
				if(this->beta.score > TEN) return this->beta.score.log(TEN);
				return ONE;
			}
			if (id == 5) {
				if(this->beta.score > TEN.pow(TEN)) return this->beta.score.log(TEN).log(TEN);
				return ONE;
			}
			return ONE;
		}

		[[nodiscard]] Number _get_BDIM() const {
			Number init = TEN;
			// Placeholder.

			if(init < ZERO) return ZERO;
			return init;
		}

		[[nodiscard]] Number _get_BDN() const {
			Number init = this->beta.decay;
			// Placeholder.
			Number dif = ONE - init;
			dif = dif.pow(_get_Bm_effect(1));
			init = ONE - dif;
			return init;
		}

		[[nodiscard]] Number _get_BDF() const {
			Number init = this->beta.decay_factor;
			Number dif = ONE - init;
			dif = dif.pow(_get_Bm_effect(2));
			init = ONE - dif;
			return init;
		}

		[[nodiscard]] Number _get_BD_step(Number matter) {
			if(matter <= TEN.pow(TEN)) return ONE;
			else if(matter > TEN.pow(TEN) && matter <= TEN.pow(TEN.pow(TEN))) return 2;
			else if(matter > TEN.pow(TEN.pow(TEN)) && matter <= TEN.pow(TEN.pow(TEN.pow(TEN)))) return 3;
			else if(matter > TEN.pow(TEN.pow(TEN.pow(TEN)))/* && matter <= TEN.pow(TEN.pow(TEN.pow(TEN.pow(TEN))))*/)
				return 4;
			return ONE;
		}

		[[nodiscard]] Number _get_BD_effect(Number matter, Number time) {
			if(!this->unlocked_beta) return ONE;
			if(matter <= TEN) return _get_BDN().pow(time);
			Number BDN = _get_BDN(), BDF = _get_BDF(), BDIM = _get_BDIM();
			Number x = matter;
			if(matter > TEN && matter <= TEN.pow(TEN)) {
				x = x.log(TEN);
			} else if(matter > TEN.pow(TEN) && matter <= TEN.pow(TEN.pow(TEN))) {
				x = x.log(TEN).log(TEN);
			} else if(matter > TEN.pow(TEN.pow(TEN)) && matter <= TEN.pow(TEN.pow(TEN.pow(TEN)))) {
				x = x.log(TEN).log(TEN).log(TEN);
			} else if(matter > TEN.pow(TEN.pow(TEN.pow(TEN)))/* && matter <= TEN.pow(TEN.pow(TEN.pow(TEN.pow(TEN))))*/) {
				x = x.log(TEN).log(TEN).log(TEN).log(TEN);
			}
			Number factor = BDN.pow((TEN - x) / 9) * BDF.pow((x - ONE) / 9);
			int step = (int)(_get_BD_step(matter)._to_long_double()) - 1;
			Number exp = 0.1 * BDIM;
			if(this->beta.weights[step] >= ZERO) exp += 0.9 * (BDIM.pow(this->beta.weights[step] + _ONE).pow(5));
			else exp += 0.9 * (BDIM.pow(this->beta.weights[step] + _ONE).pow(1));
			factor = factor.pow(exp);
			return factor.pow(time);
		}

		[[nodiscard]] Number _get_BD(Number matter, Number time) {
			if(!this->unlocked_beta) return matter;
			if(matter <= TEN) return matter * _get_BDN().pow(time);
			Number BDN = _get_BDN(), BDF = _get_BDF(), BDIM = _get_BDIM();
			Number x = matter;
			Number m = matter;
			if(m > TEN) {
				x = matter.log(TEN);
				Number factor = BDN.pow((TEN - x) / 9) * BDF.pow((x - ONE) / 9);
				Number exp = 0.1 * BDIM;
				if(this->beta.weights[0] >= ZERO) exp += 0.9 * (BDIM.pow(this->beta.weights[0] + _ONE).pow(5));
				else exp += 0.9 * (BDIM.pow((this->beta.weights[0] + _ONE).pow(Number(1))));
				factor = factor.pow(exp);
				m = m * factor.pow(time);
			}
			if(m > TEN.pow(TEN)) {
				x = matter.log(TEN).log(TEN);
				Number factor = BDN.pow((TEN - x) / 9) * BDF.pow((x - ONE) / 9);
				Number exp = 0.1 * BDIM;
				if(this->beta.weights[0] >= ZERO) exp += 0.9 * (BDIM.pow(this->beta.weights[1] + _ONE).pow(5));
				else exp += 0.9 * (BDIM.pow((this->beta.weights[0] + _ONE).pow(Number(1))));
				factor = factor.pow(exp);
				m = m.pow(factor.pow(time));
			}
			if(m > TEN.pow(TEN.pow(TEN))) {
				x = matter.log(TEN).log(TEN).log(TEN);
				Number factor = BDN.pow((TEN - x) / 9) * BDF.pow((x - ONE) / 9);
				Number exp = 0.1 * BDIM;
				if(this->beta.weights[0] >= ZERO) exp += 0.9 * (BDIM.pow(this->beta.weights[2] + _ONE).pow(5));
				else exp += 0.9 * (BDIM.pow((this->beta.weights[0] + _ONE).pow(Number(1))));
				factor = factor.pow(exp);
				m = m.dilate(factor.pow(time));
			}
			if(m > TEN.pow(TEN.pow(TEN.pow(TEN)))) {
				x = matter.log(TEN).log(TEN).log(TEN).log(TEN);
				Number factor = BDN.pow((TEN - x) / 9) * BDF.pow((x - ONE) / 9);
				Number exp = 0.1 * BDIM;
				if(this->beta.weights[0] >= ZERO) exp += 0.9 * (BDIM.pow(this->beta.weights[3] + _ONE).pow(5));
				else exp += 0.9 * (BDIM.pow((this->beta.weights[0] + _ONE).pow(Number(1))));
				factor = factor.pow(exp);
				m = m.tetrate(factor.pow(time));
			}
			return m;
		}


		// How much matter is kept.
		[[nodiscard]] Number _get_BD_kept(Number matter) {
			return _get_BD(matter, ONE);
		}

		[[nodiscard]] float _get_BE() {
			float init = 0.0f;
			if(!this->upgrade[76].level.is_zero()) init += 1.0f;
			return init;
		}


		// ------------------------- Upgrade related items.

		// Returns second scale.
		[[nodiscard]] Number _second_scale() const {
			return 4 * Number(0.99).pow(this->upgrade[25].level);
		}

		[[nodiscard]] Number _get_third_scale(const Number& s, const Number& t) const {
			static const Number THRESHOLD = TEN.pow(TEN.pow(10));

			if (t <= THRESHOLD) return s;

			Number log_s = s.log(TEN);
			if (log_s <= ZERO) return s;

			Number log_t = t.log(TEN);
			if (log_t <= ONE) return s;

			Number log_log_t = log_t.log(TEN);
			if (log_log_t <= ZERO) return s;

			Number k = log_log_t / 10;

			Number exponent = (log_s < ONE) ? (ONE / k) : k;

			Number new_log_s = log_s.pow(exponent);

			if (new_log_s < log_s) new_log_s = log_s;

			if (new_log_s < 1e-6L) new_log_s = 1e-6L;

			return _TEN.pow(new_log_s);
		}

		// Returns cost scale with given highest value. (this->alpha.highest)
		// ** NOTE: May later be discarded.
		[[nodiscard]] Number _get_scale(int id) {
			if (this->alpha.highest <= inf) return this->upgrade[id].scale;
			Number s   = this->upgrade[id].scale;
			Number emp = this->alpha.highest.log(inf);
			s = s * _second_scale().pow(emp);
			s = ONE + (s - ONE) * _get_AU_effect(3);

			s = _get_third_scale(s, this->alpha.highest);

			return s;
		}

		// Returns cost scale with cost value to be given.
		[[nodiscard]] Number _get_scale(int id, Number cost) {
			if (cost <= inf) {
				Number s = this->upgrade[id].scale;
				return _get_third_scale(s, cost);
			}
			Number s   = this->upgrade[id].scale;
			Number emp = cost.log(inf);
			if (std::isnan(emp.r.val) || std::isnan(emp.r.layer) || emp.r.neg < 0) {
				return this->upgrade[id].scale;
			}
			Number second = _second_scale();
			if (second <= ONE) return this->upgrade[id].scale;
			Number factor = second.pow(emp);
			if (factor.is_zero() || std::isnan(factor.r.val) || std::isnan(factor.r.layer)) {
				return this->upgrade[id].scale;
			}
			s = s * factor;
			s = ONE + (s - ONE) * _get_AU_effect(3);
			if (s < this->upgrade[id].scale) s = this->upgrade[id].scale;
			return s;
		}

		[[nodiscard]] Number _effective_scale(const Number& s, const Number& cost) const {
			if (cost < inf) return s;
			Number s2 = s * _second_scale().pow(cost.log(inf));
			if (cost > TEN.pow(TEN.pow(TEN))) s2 = _get_third_scale(s2, cost);
			return s2;
		}

		// Returns current cost of certain item.
		[[nodiscard]] Number _level_cost(int id, const Number& k) {
			if (id < 30) {
				Number i   = this->upgrade[id].init_cost;
				Number exp = k * k / 2 + k / 2;
				Number cost = i * this->upgrade[id].scale.pow(exp);
				Number s   = _get_scale(id, cost);
				Number final_cost = i * s.pow(exp);

				if (final_cost.is_zero() || std::isnan(final_cost.r.val) || std::isnan(final_cost.r.layer)) {
					final_cost = i * this->upgrade[id].scale.pow(exp);
				}

				s = _get_third_scale(s, final_cost);
				return i * s.pow(exp);
			}
			// BAU1-2
			if (id >= 30 && id <= 31) {
				Number i   = this->upgrade[id].init_cost;
				Number exp = k;
				return i * this->upgrade[id].scale.pow(exp);
			}
			// BAU3
			if (id == 32) {
				Number exp = 3 * Number(2).pow(k);
				return _TEN.pow(exp);
			}
			// BAU4
			if (id == 33) {
				Number exp = 5 * Number(2).pow(k);
				return _TEN.pow(exp);
			}
			// BAU5 - 7
			if (id == 34) return 1e6;
			if (id == 35) return 1e10;
			if (id == 36) return 1e15;
			if (id == 37) return 1e20;
			if (id == 38) return 1e25;
			if (id == 39) {
				Number i   = this->upgrade[id].init_cost;
				Number exp = k;
				return i * this->upgrade[id].scale.pow(exp);
			}

			// Beta Amplifiers / Beta Intensifiers
			// Beta Aeromagnifiers
			if (40 <= id && id < 76) {
				Number i   = this->upgrade[id].init_cost;
				Number exp = k * k / 2 + k / 2;
				Number cost = i * this->upgrade[id].scale.pow(exp);
				Number s   = _get_scale(id, cost);
				Number final_cost = i * s.pow(exp);

				if (final_cost.is_zero() || std::isnan(final_cost.r.val) || std::isnan(final_cost.r.layer)) {
					final_cost = i * this->upgrade[id].scale.pow(exp);
				}

				s = _get_third_scale(s, final_cost);
				return i * s.pow(exp);
			}
			if(id == 76) return _ONE;

			return ZERO;
		}

		// Approximate level M such that cumulative cost from current level x
		// up to M-1 approximately equals total resource T.
		// Formula based on cost(k) = init_cost * scale^(k(k+1)/2):
		//    M ~ (1 + sqrt(1 + 8R)) / 2,   R = log_scale(T / init_cost)
		// When cost > inf, the effective scale is larger; iterate to convergence.
		[[nodiscard]] Number _bulk_approx_level(int id, const Number& x, const Number& T) {
			if (T < _level_cost(id, x)) return x;

			Number lo = x;
			Number hi = x + 2;

			for (int rep = 0; rep < 60; ++rep) {
				if (_level_cost(id, hi) > T) break;
				Number span = hi - x;
				if (span <= ZERO) span = ONE;
				hi = x + span * 2;
				if (hi <= x) hi = x * 2;
			}

			for (int iter = 0; iter < 100; ++iter) {
				Number mid = (lo + hi) / 2;
				mid = floor(mid);
				if (mid <= lo || mid >= hi) break;
				if (_level_cost(id, mid) <= T) lo = mid;
				else                           hi = mid;
			}
			return lo;
		}

		// Upgrade ONCE if possible.
		void _try_upgrade(int id) {
			// Amp. 1-8, Int. 1-8, Amp. Aero, Int. Aero.
			if((0 <= id && id < 16) || (20 <= id && id < 22)) {
				Number l = this->upgrade[id].level;
				Number cost = _level_cost(id, l);
				if (this->pre_alpha.score >= cost) {
					this->pre_alpha.score -= cost;
					this->upgrade[id].level = level_inc(this->upgrade[id].level);
				}
			}
			// PU1-PU4.
			if(16 <= id && id < 20) {
				if (this->upgrade[id].level.is_one()) return;
				if (this->pre_alpha.score >= this->upgrade[id].init_cost) {
					this->pre_alpha.score -= this->upgrade[id].init_cost;
					this->upgrade[id].level = level_inc(this->upgrade[id].level);
				}
			}
			// AU1-AU4.
			if(22 <= id && id < 26) {
				if(this->_get_AUP_avaliable() >= ONE) this->upgrade[id].level = level_inc(this->upgrade[id].level);
			}
			// AUP Buy.
			if (id == 26) {
				Number cost = _get_AUP_cost(id);
				if (this->pre_alpha.score >= cost) {
					this->pre_alpha.score -= cost;
					this->upgrade[id].level = level_inc(this->upgrade[id].level);
				}
			}
			if (id == 27) {
				Number cost = _get_AUP_cost(id);
				if (this->upgrade[20].level >= cost) {
					this->upgrade[id].level = level_inc(this->upgrade[id].level);
				}
			}
			if (id == 28) {
				Number cost = _get_AUP_cost(id);
				if (this->upgrade[21].level >= cost) {
					this->upgrade[id].level = level_inc(this->upgrade[id].level);
				}
			}
			if (id == 29) {
				Number cost = _get_AUP_cost(id);
				if (this->alpha.point >= cost) {
					this->alpha.point -= cost;
					this->upgrade[id].level = level_inc(this->upgrade[id].level);
				}
			}
			// BAU1 / BAU2
			if(id >= 30 && id <= 31) {
				Number l = this->upgrade[id].level;
				Number cost = this->_level_cost(id, l);
				if (this->alpha.point >= cost) {
					this->alpha.point = this->alpha.point - cost;
					this->upgrade[id].level = level_inc(this->upgrade[id].level);
				}
			}
			// BAU3 / BAU4
			if (id == 32 || id == 33) {
				Number cost = this->_level_cost(id, this->upgrade[id].level);
				if (this->alpha.point >= cost) {
					this->alpha.point -= cost;
					this->upgrade[id].level = level_inc(this->upgrade[id].level);
				}
				return;
			}
			// BAU5 - BAU9 (one-time purchases)
			if (id >= 34 && id <= 38) {
				if (this->upgrade[id].level.is_one()) return; // already bought
				Number cost = this->_level_cost(id, this->upgrade[id].level);
				if (this->alpha.point >= cost) {
					this->alpha.point -= cost;
					this->upgrade[id].level = level_inc(this->upgrade[id].level);
				}
				return;
			}
			if (id == 39) {
				Number cost = this->_level_cost(id, this->upgrade[id].level);
				if (this->alpha.point >= cost) {
					this->alpha.point -= cost;
					this->upgrade[id].level = level_inc(this->upgrade[id].level);
				}
				return;
			}

			// Alpha Amplifiers / Alpha Intensifiers
			// Alpha Aeromagnifiers (56, 57), consume Am
			if (40 <= id && id < 58) {
				Number l    = this->upgrade[id].level;
				Number cost = _level_cost(id, l);
				if (this->alpha.score >= cost) {
					this->alpha.score -= cost;
					this->upgrade[id].level = level_inc(this->upgrade[id].level);
				}
				return;
			}

			if (58 <= id && id < 76) {
				Number l    = this->upgrade[id].level;
				Number cost = _level_cost(id, l);
				if (this->beta.score >= cost) {
					this->beta.score -= cost;
					this->upgrade[id].level = level_inc(this->upgrade[id].level);
				}
				return;
			}

			if (id >= 76 && id <= 76) {
				if (this->upgrade[id].level.is_one()) return; // already bought
				Number cost = this->_level_cost(id, this->upgrade[id].level);
				if (this->beta.point >= cost) {
					this->beta.point -= cost;
					this->upgrade[id].level = level_inc(this->upgrade[id].level);
				}
				return;
			}
		}

		// This allows bulk upgrade while consuming ALL certain resources.
		void _try_upgrade_bulk(int id) {
			// A1-A8, I1-I8, Amp. Aero, Int. Aero.
			// Cost formula: init_cost * scale^(k*(k+1)/2)
			if ((0 <= id && id < 16) || (20 <= id && id < 22)) {
				Number x     = this->upgrade[id].level;
				Number total = this->pre_alpha.score;
				if (total <= ZERO) {
					_try_upgrade(id);
					return;
				}

				Number first_cost = _level_cost(id, x);
				if (total < first_cost) {
					_try_upgrade(id);
					return;
				}

				Number M = _bulk_approx_level(id, x, total);

				if (M <= x) return;

				long double diff_ld = (M - x)._to_long_double();
				if (std::isfinite(diff_ld) && diff_ld > 0.5L && diff_ld < 5000.0L) {
					long long limit = (long long)diff_ld + 3;
					for (long long j = 0; j < limit; ++j) {
						Number c = _level_cost(id, this->upgrade[id].level);
						if (this->pre_alpha.score < c) break;
						this->pre_alpha.score -= c;
						this->upgrade[id].level += ONE;
					}
				}

				this->upgrade[id].level = M;
				this->pre_alpha.score   = total * 0.001;
			}

			// AU1-AU4.
			// Spend all available AUP.
			if (22 <= id && id < 26) {
				Number avail = this->_get_AUP_avaliable();
				if (avail >= ONE) {
					this->upgrade[id].level = floor(this->upgrade[id].level + avail);
				}
			}

			// AUP Buy (id 26-29)
			// 26: matter       27: Amp. Aero
			// 28: Int. Aero    29: AP
			if (id >= 26 && id <= 29) {
				if (id == 26) {
					Number T = this->pre_alpha.score;
					if (T <= ZERO) {
						_try_upgrade(id);
						return;
					}

					Number x = this->upgrade[26].level;

					Number first_cost = _get_AUP_cost(26);
					if (T < first_cost) {
						_try_upgrade(id);
						return;
					}

					// S = log10(T) / 100 = log_{1e100}(T)
					Number S = T.log(TEN) / 100;

					Number M;
					if (S <= 11) {
						M = floor(S);
					} else {
						Number disc = S * 4 - 43;
						M = floor((disc.pow(0.5) + 21) / 2);
					}

					if (M <= x) {
						_try_upgrade(id);
						return;
					}

					if (M < x + ONE) M = x + ONE;

					this->upgrade[26].level = M;
					this->pre_alpha.score = T * 0.001;
				}

				for (int loop = 0; loop < 1024; ++loop) {
					Number cost = _get_AUP_cost(id);
					bool can_buy = false;
					if (id == 27)      can_buy = (this->upgrade[20].level >= cost);
					else if (id == 28) can_buy = (this->upgrade[21].level >= cost);
					else if (id == 29) can_buy = (this->alpha.point >= cost);
					if (!can_buy) break;

					if (id == 29) this->alpha.point -= cost;
					this->upgrade[id].level = level_inc(this->upgrade[id].level);
				}
			}

			// BAU1-BAU2, BAU10 (id 30-31, 39).
			// Cost formula: init_cost * scale^k  (linear in k, not k(k+1)/2).
			// Total from x to x+n-1:
			//   init_cost * scale^x * (scale^n - 1) / (scale - 1)
			if (id == 30 || id == 31 || id == 39) {
				Number x     = this->upgrade[id].level;
				Number s     = this->upgrade[id].scale;
				Number i     = this->upgrade[id].init_cost;
				Number total = this->alpha.point;

				Number first_cost = _level_cost(id, x);
				if (!(total >= first_cost)) {
					_try_upgrade(id);
					return;
				}

				if (s == ONE) {
					// Degenerate case: linear cost
					Number n = floor(total / i);
					if (n < ONE) return;
					this->upgrade[id].level += n;
					this->alpha.point      -= n * i;
					return;
				}

				// n = log_s( total * (s - 1) / (i * s^x) + 1 )
				Number inner = total * (s - ONE) / (i * s.pow(x)) + ONE;
				if (inner <= ZERO) {
					_try_upgrade(id);
					return;
				}

				Number n = inner.log(s);
				n = floor(n);
				if (n < ONE) {
					_try_upgrade(id);
					return;
				}

				// Exact cost: sum_{k=x}^{x+n-1} i * s^k
				Number cost = i * s.pow(x) * (s.pow(n) - ONE) / (s - ONE);
				this->upgrade[id].level += n;
				this->alpha.point      -= cost;
			}

			// BAU3 / BAU4
			if (id == 32 || id == 33) {
				while (true) {
					Number cost = this->_level_cost(id, this->upgrade[id].level);
					if (this->alpha.point >= cost) {
						this->alpha.point -= cost;
						this->upgrade[id].level = level_inc(this->upgrade[id].level);
					} else {
						break;
					}
				}
			}

			// BAU5 - BAU9 (one-time)
			if (id >= 34 && id <= 38) {
				if (this->upgrade[id].level.is_one()) return;
				Number cost = this->_level_cost(id, this->upgrade[id].level);
				if (this->alpha.point >= cost) {
					this->alpha.point -= cost;
					this->upgrade[id].level = ONE;
				}
			}

			// Alpha Amplifiers / Alpha Intensifiers (40-55)
			// Alpha Amp. / Int. Aeromagnifiers (56-57)
			if (40 <= id && id < 58) {
				Number x     = this->upgrade[id].level;
				Number total = this->alpha.score;
				if (total <= ZERO) {
					_try_upgrade(id);
					return;
				}

				Number first_cost = _level_cost(id, x);
				if (total < first_cost) {
					_try_upgrade(id);
					return;
				}

				Number M = _bulk_approx_level(id, x, total);

				if (M <= x) {
					_try_upgrade(id);
					return;
				}

				long double diff_ld = (M - x)._to_long_double();
				if (std::isfinite(diff_ld) && diff_ld > 0.5L && diff_ld < 5000.0L) {
					long long limit = (long long)diff_ld + 3;
					for (long long j = 0; j < limit; ++j) {
						Number c = _level_cost(id, this->upgrade[id].level);
						if (this->alpha.score < c) break;
						this->alpha.score -= c;
						this->upgrade[id].level += ONE;
					}
				}

				this->upgrade[id].level = M;
				this->alpha.score   = total * 0.001;
			}

			// Beta Amplifiers / Beta Intensifiers (58-73)
			// Beta Amp. / Int. Aeromagnifiers (74-75)
			if (58 <= id && id < 76) {
				Number x     = this->upgrade[id].level;
				Number total = this->beta.score;
				if (total <= ZERO) {
					_try_upgrade(id);
					return;
				}

				Number first_cost = _level_cost(id, x);
				if (total < first_cost) {
					_try_upgrade(id);
					return;
				}

				Number M = _bulk_approx_level(id, x, total);

				if (M <= x) {
					_try_upgrade(id);
					return;
				}

				long double diff_ld = (M - x)._to_long_double();
				if (std::isfinite(diff_ld) && diff_ld > 0.5L && diff_ld < 5000.0L) {
					long long limit = (long long)diff_ld + 3;
					for (long long j = 0; j < limit; ++j) {
						Number c = _level_cost(id, this->upgrade[id].level);
						if (this->beta.score < c) break;
						this->beta.score -= c;
						this->upgrade[id].level += ONE;
					}
				}

				this->upgrade[id].level = M;
				this->beta.score   = total * 0.001;
			}

			if (id >= 76 && id <= 76) {
				if (this->upgrade[id].level.is_one()) return;
				Number cost = this->_level_cost(id, this->upgrade[id].level);
				if (this->beta.point >= cost) {
					this->beta.point -= cost;
					this->upgrade[id].level = ONE;
				}
			}
			_try_upgrade(id);
		}
		// Upgrades all pre-prestige upgrades(or max all).
		// Includes A1-8,I1-8,AA1-AA8,AI1-AI8.
		void _try_upgrade_bulk_all() {
			Number cur = this->pre_alpha.score;
			for (int i = 0; i < 16; i ++) {
				this->pre_alpha.score = cur;
				_try_upgrade(i);
				_try_upgrade_bulk(i);
				if(cur < 1e100) cur = this->pre_alpha.score;
			}
			cur = this->alpha.score;
			if (!this->upgrade[34].level.is_zero()) {
				for (int i = 0; i < 16; i ++) {
					this->alpha.score = cur;
					_try_upgrade(i + 40);
					_try_upgrade_bulk(i + 40);
					if(cur < 1e100) cur = this->alpha.score;
				}
			}
		}

		[[nodiscard]] Number _get_game_speed() {
			Number base = _accel;
			return base;
		}
};

player_ player;
long double _get_time();

bool simulator_::_get_ach(int id){
	return player._get_ach(id);
}

Number simulator_::_get_SP_effect(int id){
	return player._get_SP_effect(id);
}

// Initialize the cost.
void cost_init(player_& player) {
	for (int i = 0; i < 8; i ++) {
		player.upgrade[i].scale = 1.5;
		player.upgrade[i].init_cost = TEN.pow(3 * i * i);
		player.upgrade[i + 8].scale = 3;
		player.upgrade[i + 8].init_cost = 100 * TEN.pow(3 * i * i);
		player.upgrade[40 + i].scale = 1.5;
		player.upgrade[40 + i].init_cost = 100 * TEN.pow(3 * i * i);
		player.upgrade[48 + i].scale = 3;
		player.upgrade[48 + i].init_cost = 10000 * TEN.pow(3 * i * i);
		player.upgrade[58 + i].scale = 1.5;
		player.upgrade[58 + i].init_cost = 1000 * TEN.pow(3 * i * i);
		player.upgrade[66 + i].scale = 3;
		player.upgrade[66 + i].init_cost = 100000 * TEN.pow(3 * i * i);
	}
	player.upgrade[16].init_cost = 1e16;
	player.upgrade[17].init_cost = 1e30;
	player.upgrade[18].init_cost = 1e50;
	player.upgrade[19].init_cost = 1e100;
	player.upgrade[20].scale = 1e3;
	player.upgrade[20].init_cost = 1e10;
	player.upgrade[21].scale = 1e5;
	player.upgrade[21].init_cost = 1e20;

	player.upgrade[30].scale = TEN;
	player.upgrade[30].init_cost = TEN;
	player.upgrade[31].scale = TEN;
	player.upgrade[31].init_cost = 100;
	player.upgrade[32].scale = 2;
	player.upgrade[32].init_cost = 1e3;
	player.upgrade[33].scale = 2;
	player.upgrade[33].init_cost = 1e5;
	player.upgrade[34].scale = ONE;
	player.upgrade[34].init_cost = 1e6;
	player.upgrade[35].scale = ONE;
	player.upgrade[35].init_cost = 1e10;
	player.upgrade[36].init_cost = 1e15;
	player.upgrade[36].scale = ONE;
	player.upgrade[37].init_cost = 1e20;
	player.upgrade[37].scale = ONE;
	player.upgrade[38].init_cost = 1e25;
	player.upgrade[38].scale = ONE;
	player.upgrade[39].init_cost = 1e30;
	player.upgrade[39].scale = 100;

	player.upgrade[56].scale = 1e3;
	player.upgrade[56].init_cost = 1e10;
	player.upgrade[57].scale = 1e5;
	player.upgrade[57].init_cost = 1e20;
	player.upgrade[74].scale = 1e3;
	player.upgrade[74].init_cost = 1e10;
	player.upgrade[75].scale = 1e5;
	player.upgrade[75].init_cost = 1e20;
}


// Initialization.
void init(player_& player) {
	player.pre_alpha.score = ONE;
	cost_init(player);

	for (int i = 0; i < 1000; ++i) {
		player.ach[i] = false;
		player.story[i] = false;
	}

	player.GIM = TEN;						// Initial GIM.
	player.PIM = TEN;						// Initial PIM.
	player.PF = TEN;						// Initial PF.
	player.pre_alpha.highest = ONE;		    // Initial score. (for prestige)
	player.last_tick = _get_time();			// Time init.
	player.currenttab = 1;					// Tab init.
	player.unlockedtabs = 3;
	player.unlocked_prestige = false;		// Self-explanatory.

	player.time_flux = 0;
	player.pre_alpha.time = ZERO;
	player.alpha.time = ZERO;
	player.fastest_ap_per_min = ZERO;
	player.alpha.score   = ONE;
	player.alpha.am_best = ONE;
	player.alpha.am_rate = ZERO;
	player.alpha.AIM     = TEN;
	player.beta.score   = ONE;
	player.beta.time = ZERO;
	player.alpha.simulator = {};
}

// Initalization on an Aeromagnifier reset.
void init_aero(player_& player) {
	player.pre_alpha.time = ZERO;
	if(player._get_AM(0)) return;
	if(player.upgrade[19].level.is_one()) player.pre_alpha.score = TEN * TEN * TEN * TEN; //** PU4: Start with 10000.00 matter.
	else player.pre_alpha.score = ONE;
	for (int i = 0; i < 8; i ++) {
		player.pre_alpha.amount[i] = ONE;
		player.upgrade[i].level = ZERO;
		player.upgrade[i + 8].level = ZERO;
	}
}

// Initalization on an Alpha Aeromagnifier reset.
void init_alpha_aero(player_& player) {
	player.alpha.score = TEN * TEN;
	for (int i = 0; i < 8; i ++) {
		player.alpha.amount[i] = ONE;
		player.upgrade[i + 40].level = ZERO;
		player.upgrade[i + 48].level = ZERO;
	}
}

// Initialization after prestige once.
void init_prestige(player_& player) {
	if(player.pre_alpha.score > player.pre_alpha.highest) player.pre_alpha.highest = player.pre_alpha.score; //Update highest score, for recalculate GIM, etc.
	init_aero(player);
	cost_init(player);
	if(player.upgrade[19].level.is_one()) player.pre_alpha.score = TEN * TEN * TEN * TEN; //** PU4: Start with 10000.00 matter.
	else player.pre_alpha.score = ONE;
	for (int i = 0; i < 8; i ++) {
		player.pre_alpha.amount[i] = ONE;
		player.upgrade[i].level = ZERO;
		player.upgrade[i + 8].level = ZERO;
	}
	player.upgrade[20].level = ZERO;
	player.upgrade[21].level = ZERO;
	player.pre_alpha.time = ZERO;
	player.unlockedtabs = max(player.unlockedtabs, 4);
}

// Initialization after (performing) alpha once.
void init_alpha(player_& player) {
	Number ap_gained = player._get_AP();
	player.alpha.point += ap_gained;
	player.alpha.alpha += player._get_alpha();

	// Update AP/min
	Number seconds = player.alpha.time;
	if (seconds > ZERO) {
		Number ap_per_min = ap_gained * 60 / seconds;
		if (ap_per_min > player.fastest_ap_per_min) {
			player.fastest_ap_per_min = ap_per_min;
		}
	}

	init_prestige(player);
	player.pre_alpha.score = ONE;
	for(int i = 16; i < 20; i ++) {
		if(!player._get_AM(4)) player.upgrade[i].level = ZERO;
	}
	for(int i = 20; i < 22; i ++) {
		player.upgrade[i].level = ZERO;
	}

	player.last_tick = _get_time();
	player.unlockedtabs = max(player.unlockedtabs, 5);
	player.unlocked_alpha = true;
	player.alpha.time = ZERO;
	player.pre_alpha.time = ZERO;
	player.pre_alpha.highest = ONE;
}

// Initialization after RESPECCING alpha once. (AUP respec, etc.)
// Notice that respeccing isn't equal to performing an alpha, or any respective prestige layer.
// While performing an alpha gives you reward, this doesn't give you any reward from your current alpha.
// It seems that respeccing is a waste of time, but it will be widely used for various strategies.
void respec_alpha(player_& player) {
	// player.alpha.point += player._get_AP();
	// player.alpha.alpha += player._get_alpha();
	init_prestige(player);
	player.pre_alpha.score = ONE;
	for(int i = 16; i < 20; i ++) {
		if(!player._get_AM(4)) player.upgrade[i].level = ZERO;
	}
	for(int i = 20; i < 22; i ++) {
		player.upgrade[i].level = ZERO;
	}

	player.last_tick = _get_time();
	player.unlockedtabs = max(player.unlockedtabs, 5);
	player.unlocked_alpha = true;
	player.alpha.time = ZERO;
	player.pre_alpha.time = ZERO;
	player.pre_alpha.highest = ONE;
}

void init_beta(player_& player) {
	Number bp_gained = player._get_BP();
	player.beta.point += bp_gained;
	player.beta.beta += player._get_beta();

	// Update BP/min
	Number seconds = player.beta.time;
	if (seconds > ZERO) {
		Number bp_per_min = bp_gained * 60 / seconds;
		if (bp_per_min > player.fastest_bp_per_min) {
			player.fastest_bp_per_min = bp_per_min;
		}
	}

	init_alpha(player);
	player.alpha.score = ONE;
	for(int i = 0; i < 58; i ++) {
		player.upgrade[i].level = ZERO;
	}
	for(int i = 0; i < 8; i ++) {
		player.alpha.amount[i] = ONE;
	}
	player.upgrade[34].level = ONE;
	player.upgrade[35].level = ONE;
	for(int i = 1; i <= 52; i ++) {
		player.ach[i] = false;
	}

	player.last_tick = _get_time();
	player.unlockedtabs = max(player.unlockedtabs, 6);
	player.unlocked_alpha = true;
	player.break_alpha = false;
	player.unlocked_beta = true;
	player.alpha.highest = ONE;
	player.alpha.am_best = ONE;
	player.alpha.am_rate = ONE;
	player.alpha.alpha = ZERO;
	player.alpha.point = ZERO;
	player.beta.highest = ONE;
	player.beta.highest_am = ONE;

	player.beta.time = ZERO;
	player.fastest_ap_per_min = ZERO;
}

// ============================================================
// Section 5 - Audio initialization.
// ============================================================

const vector<string> BGM_LIST = {
	"The Beginning.mp3",
	"Les Route.mp3",
	"Stars.mp3"
};

static int current_bgm_index = -1;
static int current_bgm_volume = -1;

string get_bgm_filename(int index) {
	if (index < 0 || index >= (int)BGM_LIST.size()) return "";
	return BGM_LIST[index];
}

string get_bgm_name(int index) {
	string f = get_bgm_filename(index);
	if (f.empty()) return "Unknown";
	if (f.size() > 4 && f.substr(f.size() - 4) == ".mp3")
		return f.substr(0, f.size() - 4);
	return f;
}

void play_bgm(int index, int volume) {
	mciSendStringW(L"close bgm", NULL, 0, NULL);
	string filename = get_bgm_filename(index);
	if (filename.empty()) return;

	wstring wfile(filename.begin(), filename.end());
	wstring cmd = L"open \"" + wfile + L"\" type mpegvideo alias bgm";
	if (mciSendStringW(cmd.c_str(), NULL, 0, NULL) != 0) return;

	mciSendStringW(L"play bgm repeat", NULL, 0, NULL);

	if (volume < 0) volume = 0;
	if (volume > 100) volume = 100;
	wstring vol_cmd = L"setaudio bgm volume to " + to_wstring(volume * 10);
	mciSendStringW(vol_cmd.c_str(), NULL, 0, NULL);
}

void set_bgm_volume(int volume) {
	if (volume < 0) volume = 0;
	if (volume > 100) volume = 100;
	wstring cmd = L"setaudio bgm volume to " + to_wstring(volume * 10);
	mciSendStringW(cmd.c_str(), NULL, 0, NULL);
}

void stop_bgm() {
	mciSendStringW(L"close bgm", NULL, 0, NULL);
}

// ============================================================
// Section 6 - GUI related items initialization.
// ============================================================

static ImVec4 _get_BD_color() {
	long double t = _get_time() / 1000.0;
	return ImVec4(1.0f, 1.0f-0.425f-0.425*sin(t), 0.5f-0.5*sin(t), 1.0f);
}

bool editing_option = false;

//GUI Items start here.
#ifdef _WIN32

// Notification / Popup system
// Implements AD-style achievement popups at the top-right corner.

struct Notification {
	std::string title;
	std::string message;
	ImVec4      accent;          // border / title color
	float       elapsed    = 0.0f;
	float       duration   = 3.4f;   // total lifetime in seconds
	float       slide_time = 0.35f;  // slide in / slide out duration
	int         id         = 0;      // stable id for ImGui window name
	bool        active     = true;
};

// Global container. Kept simple: a plain vector, no threads.
static std::vector<Notification> g_notifications;
static int                        g_notification_next_id = 0;

// Layout constants
static const float NOTIF_WIDTH    = 340.0f; // fixed width of a popup
static const float NOTIF_SPACING  = 8.0f;   // vertical gap between popups
static const float NOTIF_MARGIN   = 16.0f;  // margin from screen edges
static const int   NOTIF_MAX_ONSCREEN = 5;  // max simultaneously shown

// Push a new notification. Called from _unlock / _unlock_story.
inline void PushNotification(const std::string& title,
                             const std::string& message,
                             const ImVec4& accent = ImVec4(0.35f, 1.0f, 0.45f, 1.0f)) {
	// Cap the queue so a burst of unlocks doesn't flood the screen.
	if ((int)g_notifications.size() >= NOTIF_MAX_ONSCREEN) {
		// Drop the oldest one immediately.
		g_notifications.erase(g_notifications.begin());
	}
	Notification n;
	n.title   = title;
	n.message = message;
	n.accent  = accent;
	n.id      = g_notification_next_id++;
	g_notifications.push_back(n);
}

// Render all notifications. Call this inside GUI_RenderFrame(),
// BEFORE ImGui::Render(), and AFTER all the tab windows.
inline void RenderNotifications() {
	if (g_notifications.empty()) return;

	ImGuiViewport* vp = ImGui::GetMainViewport();
	const float base_x = vp->WorkPos.x + vp->WorkSize.x - NOTIF_WIDTH - NOTIF_MARGIN;
	// Start below the top bar (top bar is 42px tall in this project).
	float y_cursor = vp->WorkPos.y + 42.0f + NOTIF_MARGIN;

	const float dt = ImGui::GetIO().DeltaTime;

	for (auto& n : g_notifications) {
		if (!n.active) continue;

		// --- Animation ------------------------------------------------
		float alpha    = 1.0f;
		float x_offset = 0.0f;

		if (n.elapsed < n.slide_time) {
			// Slide-in: ease-out
			float t    = n.elapsed / n.slide_time;
			float ease = 1.0f - (1.0f - t) * (1.0f - t);
			alpha      = ease;
			x_offset   = (1.0f - ease) * (NOTIF_WIDTH + NOTIF_MARGIN);
		} else if (n.duration - n.elapsed < n.slide_time) {
			// Slide-out: ease-in
			float t    = (n.duration - n.elapsed) / n.slide_time;
			float ease = t * t;
			alpha      = ease;
			x_offset   = (1.0f - ease) * (NOTIF_WIDTH + NOTIF_MARGIN);
		}

		ImGui::SetNextWindowPos(ImVec2(base_x + x_offset, y_cursor), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(NOTIF_WIDTH, 0.0f), ImGuiCond_Always);

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.09f, 0.09f, 0.12f, 0.96f));
		ImGui::PushStyleColor(ImGuiCol_Border,   n.accent);

		ImGuiWindowFlags flags =
		    ImGuiWindowFlags_NoDecoration        |
		    ImGuiWindowFlags_NoInputs            | // no mouse focus
		    ImGuiWindowFlags_NoNavFocus          | // no keyboard focus
		    ImGuiWindowFlags_NoSavedSettings     |
		    ImGuiWindowFlags_NoFocusOnAppearing  |
		    ImGuiWindowFlags_AlwaysAutoResize;

		std::string win_name = "##notification_" + std::to_string(n.id);
		ImGui::Begin(win_name.c_str(), nullptr, flags);

		// Title
		ImGui::PushStyleColor(ImGuiCol_Text, n.accent);
		ImGui::TextUnformatted(n.title.c_str());
		ImGui::PopStyleColor();

		// Thin separator line in accent color
		ImGui::PushStyleColor(ImGuiCol_Separator, n.accent);
		ImGui::Separator();
		ImGui::PopStyleColor();

		// Body
		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + NOTIF_WIDTH - 24.0f);
		ImGui::TextUnformatted(n.message.c_str());
		ImGui::PopTextWrapPos();

		// We need the actual height for stacking.
		float win_height = ImGui::GetWindowSize().y;

		ImGui::End();

		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(4);

		// Advance cursor for the next popup
		y_cursor += win_height + NOTIF_SPACING;

		// Update lifetime
		n.elapsed += dt;
		if (n.elapsed >= n.duration) n.active = false;
	}

	// Cleanup finished popups
	g_notifications.erase(
	    std::remove_if(g_notifications.begin(), g_notifications.end(),
	[](const Notification& n) {
		return !n.active;
	}),
	g_notifications.end());
}

static ID3D11ShaderResourceView* g_splash_srv = nullptr;
static bool   g_splash_done      = false;
static ImFont* g_font_small      = nullptr;      // smaller font for on-ball life text
static double g_splash_start     = 0.0;
static const double SPLASH_DURATION_MS = 1500.0;
static const double SPLASH_FADE_MS     = 400.0;


void CreateRenderTarget() {
	ID3D11Texture2D* pBackBuffer = nullptr;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	if (pBackBuffer) {
		g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
		pBackBuffer->Release();
	}
}

void CleanupRenderTarget() {
	if (g_mainRenderTargetView) {
		g_mainRenderTargetView->Release();
		g_mainRenderTargetView = nullptr;
	}
}

bool CreateDeviceD3D(HWND hWnd) {
	DXGI_SWAP_CHAIN_DESC sd {};
	sd.BufferCount = 2;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL levels[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
	              levels, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED)
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
		                                    levels, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res != S_OK) return false;
	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D() {
	CleanupRenderTarget();
	if (g_pSwapChain)        {
		g_pSwapChain->Release();
		g_pSwapChain = nullptr;
	}
	if (g_pd3dDeviceContext) {
		g_pd3dDeviceContext->Release();
		g_pd3dDeviceContext = nullptr;
	}
	if (g_pd3dDevice)        {
		g_pd3dDevice->Release();
		g_pd3dDevice = nullptr;
	}
}

static ID3D11ShaderResourceView* LoadTextureFromFile(const wchar_t* filename) {
	if (!g_pd3dDevice) return nullptr;

	// 1. Init. WIC.
	IWICImagingFactory* factory = nullptr;
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	HRESULT hr = CoCreateInstance(
	                 CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
	                 IID_PPV_ARGS(&factory));
	if (FAILED(hr)) return nullptr;

	// 2. Open file and decode.
	IWICBitmapDecoder* decoder = nullptr;
	hr = factory->CreateDecoderFromFilename(
	         filename, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
	if (FAILED(hr)) {
		factory->Release();
		return nullptr;
	}

	IWICBitmapFrameDecode* frame = nullptr;
	hr = decoder->GetFrame(0, &frame);
	if (FAILED(hr)) {
		decoder->Release();
		factory->Release();
		return nullptr;
	}

	// 3. To 32-bit RGBA.
	IWICFormatConverter* converter = nullptr;
	hr = factory->CreateFormatConverter(&converter);
	if (FAILED(hr)) {
		frame->Release();
		decoder->Release();
		factory->Release();
		return nullptr;
	}

	hr = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
	                           WICBitmapDitherTypeNone, nullptr, 0.0,
	                           WICBitmapPaletteTypeCustom);
	if (FAILED(hr)) {
		converter->Release();
		frame->Release();
		decoder->Release();
		factory->Release();
		return nullptr;
	}

	UINT w = 0, h = 0;
	converter->GetSize(&w, &h);
	std::vector<BYTE> pixels(w * h * 4);
	hr = converter->CopyPixels(nullptr, w * 4, (UINT)pixels.size(), pixels.data());

	converter->Release();
	frame->Release();
	decoder->Release();
	factory->Release();

	if (FAILED(hr)) return nullptr;

	// 4. Create D3D11 texture.
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = w;
	desc.Height = h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA sub = {};
	sub.pSysMem = pixels.data();
	sub.SysMemPitch = w * 4;

	ID3D11Texture2D* tex = nullptr;
	hr = g_pd3dDevice->CreateTexture2D(&desc, &sub, &tex);
	if (FAILED(hr)) return nullptr;

	ID3D11ShaderResourceView* srv = nullptr;
	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = desc.Format;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;
	hr = g_pd3dDevice->CreateShaderResourceView(tex, &srv_desc, &srv);
	tex->Release();
	if (FAILED(hr)) return nullptr;
	return srv;
}

void save();

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
	switch (msg) {
		case WM_SIZE:
			if (wParam == SIZE_MINIMIZED) return 0;
			g_ResizeWidth = (UINT)LOWORD(lParam);
			g_ResizeHeight = (UINT)HIWORD(lParam);
			return 0;
		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
			break;
		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;
		case WM_CLOSE:
			if (g_use_gui) {
				save();
				stop_bgm();
			}
			::DestroyWindow(hWnd);
			return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool GUI_Init() {
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
	                   GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
	                   L"ProjectSimulation", nullptr
	                 };
	::RegisterClassExW(&wc);

	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Project: Simulation v1.0.1: The Alpha Revamp Update.",
	                            WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800,
	                            nullptr, nullptr, wc.hInstance, nullptr);

	if (!hwnd) {
		cout << "[ERROR] Failed to create Window." << endl;
		return false;
	}

	if (!CreateDeviceD3D(hwnd)) {
		cout << "[ERROR] Failed to create D3D Device." << endl;
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		::DestroyWindow(hwnd);
		return false;
	}

	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	g_splash_srv = LoadTextureFromFile(L"start.png");
	g_splash_start = _get_time();
	g_splash_done = (g_splash_srv == nullptr);

	ImGuiIO& io = ImGui::GetIO();

	ImFont* font_normal = nullptr;
	if (std::filesystem::exists("consola.ttf")) {
		font_normal = io.Fonts->AddFontFromFileTTF("consola.ttf", 14.0f);
		g_font_small  = io.Fonts->AddFontFromFileTTF("consola.ttf", 9.0f);
	} else {
		cout << "[WARNING] consola.ttf not found. Using default font." << endl;
	}

	if (font_normal) io.FontDefault = font_normal;

	g_gui_running = true;
	g_gui_active = true;
	return true;
}
void GUI_Shutdown() {
	if (!g_gui_active) return;
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	CleanupDeviceD3D();
	if (g_splash_srv) {
		g_splash_srv->Release();
		g_splash_srv = nullptr;
	}
	g_gui_active = false;
}

// Alpha button styling helpers

// Red theme
static const ImVec4 ALPHA_BTN_BG      = ImVec4(0.28f, 0.04f, 0.07f, 1.00f); // Dark Red.
static const ImVec4 ALPHA_BTN_HOVER   = ImVec4(0.58f, 0.10f, 0.14f, 1.00f); // Hover: lighter
static const ImVec4 ALPHA_BTN_ACTIVE  = ImVec4(0.14f, 0.02f, 0.04f, 1.00f); // Pressed: Darker
static const ImVec4 ALPHA_BTN_TEXT    = ImVec4(1.00f, 0.85f, 0.85f, 1.00f); // Red-White text.
static const ImVec4 ALPHA_BTN_BORDER  = ImVec4(0.75f, 0.25f, 0.30f, 1.00f); // Border Red.

// Call before drawing Alpha Button
inline void PushAlphaButtonStyle() {
	ImGui::PushStyleColor(ImGuiCol_Button,        ALPHA_BTN_BG);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ALPHA_BTN_HOVER);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ALPHA_BTN_ACTIVE);
	ImGui::PushStyleColor(ImGuiCol_Text,          ALPHA_BTN_TEXT);
	ImGui::PushStyleColor(ImGuiCol_Border,        ALPHA_BTN_BORDER);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
}

// Call after drawing Alpha.
inline void PopAlphaButtonStyle() {
	ImGui::PopStyleVar(1);
	ImGui::PopStyleColor(5);
}

// Encapsulation of a simplified Alpha Button.
inline bool AlphaButton(const char* label, const ImVec2& size = ImVec2(0, 0)) {
	PushAlphaButtonStyle();
	bool clicked = ImGui::Button(label, size);
	PopAlphaButtonStyle();
	return clicked;
}

// Beta button styling

// Orange theme
static const ImVec4 BETA_BTN_BG      = ImVec4(0.36f, 0.20f, 0.04f, 1.00f); // Dark Red.
static const ImVec4 BETA_BTN_HOVER   = ImVec4(0.58f, 0.30f, 0.14f, 1.00f); // Hover: lighter
static const ImVec4 BETA_BTN_ACTIVE  = ImVec4(0.21f, 0.12f, 0.02f, 1.00f); // Pressed: Darker
static const ImVec4 BETA_BTN_TEXT    = ImVec4(1.00f, 0.92f, 0.85f, 1.00f); // Red-White text.
static const ImVec4 BETA_BTN_BORDER  = ImVec4(0.75f, 0.38f, 0.30f, 1.00f); // Border Red.

inline void PushBetaButtonStyle() {
	ImGui::PushStyleColor(ImGuiCol_Button,        BETA_BTN_BG);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, BETA_BTN_HOVER);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,  BETA_BTN_ACTIVE);
	ImGui::PushStyleColor(ImGuiCol_Text,          BETA_BTN_TEXT);
	ImGui::PushStyleColor(ImGuiCol_Border,        BETA_BTN_BORDER);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
}

inline void PopBetaButtonStyle() {
	ImGui::PopStyleVar(1);
	ImGui::PopStyleColor(5);
}

inline bool BetaButton(const char* label, const ImVec2& size = ImVec2(0, 0)) {
	PushBetaButtonStyle();
	bool clicked = ImGui::Button(label, size);
	PopBetaButtonStyle();
	return clicked;
}


//Supplementary function: Number -> String
static string N2S(const Number& n) {
	return n.normalize().toString();
}

//Init. draw tabs.
void DrawMatterTab(player_& player);
void DrawOptionsTab(player_& player);
void DrawPrestigeTab(player_& player);
void DrawTopBar(player_& player);
void DrawAlphaTab(player_& player);
void DrawStoryTab(player_& player);

// A1-A8 / I1-I8 Buy mode switch
// true  = Buy Max
// false = Buy 1
static bool g_bulk_buy = true;

// Static holder for calculation results.
static Number m_rate[8] = {ZERO,ZERO,ZERO,ZERO,ZERO,ZERO,ZERO,ZERO};

//An upgrade card includes A(i+1) and I(i+1).
void DrawUpgradeCard(player_& player, int i) {
	if (i % 4 != 0) ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::BeginChild(("##upg" + to_string(i)).c_str(), ImVec2(300, 200), true);

	ImGui::Text("M%d = %s", i + 1, N2S(player.pre_alpha.amount[i]).c_str());
	ImGui::Separator();

	// Amplifier
	Number a = 3
	           * player.upgrade[i].level
	           * player._get_aero_effect_a()
	           * player._get_ach_effect(1)
	           * player._get_BAU_effect(1)
	           * player._get_Am_effect(3);
	if(player.upgrade[17].level.is_one()) a = a * player._get_PU2_effect();
	if(player._get_ach(32)) a = a * player._get_PU2_effect(player.pre_alpha.highest);
	if(player._get_story(1)) a = a * player._get_story_effect(1);
	Number lev = player.upgrade[i + 8].level + ONE;
	Number exp = lev.pow_sum(player._get_GIM_effect() - 1)
	             * player._get_aero_effect_i()
	             * player._get_Am_effect(4);

	Number effect = a.pow(exp);

	m_rate[i] = effect;
	ImGui::Text("%s A%d",N2S(player.upgrade[i].level).c_str(), i + 1);
	ImGui::Text("Produce %s M%d/s", N2S(effect).c_str(), i + 1);
	ImGui::Text("Cost: %s", N2S(player._level_cost(i, player.upgrade[i].level)).c_str());

	const char* a_label = g_bulk_buy
	                      ? (player._get_AM(1) ? "Max A" : "Buy Max A")
		                      : "Buy A";
	string a_btn = string(a_label) + to_string(i + 1);

	if(!player._get_AM(1)) {
		if(ImGui::Button(a_btn.c_str(), ImVec2(120, 0))) {
			if (g_bulk_buy) player._try_upgrade_bulk(i);
			Number x = player.upgrade[i].level;
			player._try_upgrade(i);
			while(x != player.upgrade[i].level) {
				x = player.upgrade[i].level;
				player._try_upgrade(i);
			}
		}
	} else {
		if(ImGui::Button(a_btn.c_str())) {
			if (g_bulk_buy) player._try_upgrade_bulk(i);
			Number x = player.upgrade[i].level;
			player._try_upgrade(i);
			while(x != player.upgrade[i].level) {
				x = player.upgrade[i].level;
				player._try_upgrade(i);
			}
		}
		ImGui::SameLine();
		string en = (player.upgrade[i].autobuyer.enabled) ? "Auto: ON" : "Auto: OFF";
		if(ImGui::Button((en + "##autoamp" + to_string(i)).c_str())) player.upgrade[i].autobuyer.enabled ^= 1;
	}

	ImGui::Separator();

	// Intensifier
	ImGui::Text("%s I%d", N2S(player.upgrade[i + 8].level).c_str(), i + 1);
	ImGui::Text("Empower M/s to ^%s", N2S(exp).c_str());
	ImGui::Text("Cost: %s", N2S(player._level_cost(i + 8, player.upgrade[i + 8].level)).c_str());

	const char* i_label = g_bulk_buy
	                      ? (player._get_AM(2) ? "Max I" : "Buy Max I")
		                      : "Buy I";
	string i_btn = string(i_label) + to_string(i + 1);

	if(!player._get_AM(2)) {
		if(ImGui::Button(i_btn.c_str(), ImVec2(120, 0))) {
			if (g_bulk_buy) player._try_upgrade_bulk(i + 8);
			Number x = player.upgrade[i + 8].level;
			player._try_upgrade(i + 8);
			while(x != player.upgrade[i + 8].level) {
				x = player.upgrade[i + 8].level;
				player._try_upgrade(i + 8);
			}
		}
	} else {
		if(ImGui::Button(i_btn.c_str())) {
			if (g_bulk_buy) player._try_upgrade_bulk(i + 8);
			Number x = player.upgrade[i + 8].level;
			player._try_upgrade(i + 8);
			while(x != player.upgrade[i + 8].level) {
				x = player.upgrade[i + 8].level;
				player._try_upgrade(i + 8);
			}
		}
		ImGui::SameLine();
		string en = (player.upgrade[i + 8].autobuyer.enabled) ? "Auto: ON" : "Auto: OFF";
		if(ImGui::Button((en + "##autoint" + to_string(i)).c_str())) player.upgrade[i + 8].autobuyer.enabled ^= 1;
	}

	ImGui::EndChild();
	ImGui::EndGroup();
}

void DrawAlphaUpgradeCard(player_& player, int i) {
	if (i % 4 != 0) ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::BeginChild(("##aupg" + to_string(i)).c_str(), ImVec2(300, 200), true);

	ImGui::Text("AM%d = %s", i + 1, N2S(player.alpha.amount[i]).c_str());
	ImGui::Separator();

	// Alpha Amplifier
	Number aa_base = 3
	                 * player.upgrade[40 + i].level
	                 * player._get_alpha_aero_effect_a()
	                 * player._get_story_effect(3);

	if (player.upgrade[36].level.is_one()) {
		aa_base = aa_base * player._get_ach_effect(1);
	}
	if (player._get_SM(1)) {
		aa_base = aa_base * player._get_story_effect(1);
	}
	if (player._get_SM(7)) {
		Number base = player.pre_alpha.score;
		Number mult = ONE;
		if(base >= TEN) {
			Number base_log = base.log(TEN);
			if(base_log >= TEN) {
				mult = base_log.log(TEN);
			}
		}
		aa_base = aa_base * mult;
	}

	Number lev = player.upgrade[48 + i].level + ONE;
	Number exp = lev.pow_sum(player._get_AIM_effect() - 1)
	             * player._get_alpha_aero_effect_i();
	Number effect = aa_base.pow(exp);

	ImGui::Text("%s AA%d", N2S(player.upgrade[40 + i].level).c_str(), i + 1);
	ImGui::Text("Produce %s AM%d/s", N2S(effect).c_str(), i + 1);
	ImGui::Text("Cost: %s Am",
	            N2S(player._level_cost(40 + i, player.upgrade[40 + i].level)).c_str());

	const char* a_label = g_bulk_buy
	                      ? (false ? "Max AA" : "Buy Max AA") //The false here is for space of future AA/AI Autobuyers.
		                      : "Buy AA";
	string a_btn = string(a_label) + to_string(i + 1);

	if(AlphaButton(a_btn.c_str(), ImVec2(120, 0))) {
		if (g_bulk_buy) player._try_upgrade_bulk(i + 40);
		Number x = player.upgrade[i + 40].level;
		player._try_upgrade(i + 40);
		while(x != player.upgrade[i + 40].level) {
			x = player.upgrade[i + 40].level;
			player._try_upgrade(i + 40);
		}
	}
	ImGui::Separator();

	// Alpha Intensifier
	ImGui::Text("%s AI%d", N2S(player.upgrade[48 + i].level).c_str(), i + 1);
	ImGui::Text("Empower AM/s to ^%s", N2S(exp).c_str());
	ImGui::Text("Cost: %s Am",
	            N2S(player._level_cost(48 + i, player.upgrade[48 + i].level)).c_str());
	const char* i_label = g_bulk_buy
	                      ? (false ? "Max AI" : "Buy Max AI") //The false here is for space of future AA/AI Autobuyers.
		                      : "Buy AI";
	string i_btn = string(i_label) + to_string(i + 1);

	if(AlphaButton(i_btn.c_str(), ImVec2(120, 0))) {
		if (g_bulk_buy) player._try_upgrade_bulk(i + 48);
		Number x = player.upgrade[i + 48].level;
		player._try_upgrade(i + 48);
		while(x != player.upgrade[i + 48].level) {
			x = player.upgrade[i + 48].level;
			player._try_upgrade(i + 48);
		}
	}

	ImGui::EndChild();
	ImGui::EndGroup();
}

void DrawBetaUpgradeCard(player_& player, int i) {
	if (i % 4 != 0) ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::BeginChild(("##bupg" + to_string(i)).c_str(), ImVec2(300, 200), true);

	ImGui::Text("BM%d = %s", i + 1, N2S(player.beta.amount[i]).c_str());
	ImGui::Separator();

	// Beta Amplifier
	Number ba_base = 3
	                 * player.upgrade[58 + i].level
	                 * player._get_story_effect(3);

	if (player._get_SM(1)) {
		ba_base = ba_base * player._get_story_effect(1);
	}
	if (player._get_SM(7)) {
		Number base = player.pre_alpha.score;
		Number mult = ONE;
		if(base >= TEN) {
			Number base_log = base.log(TEN);
			if(base_log >= TEN) {
				mult = base_log.log(TEN);
			}
		}
		ba_base = ba_base * mult;
	}

	Number lev = player.upgrade[66 + i].level + ONE;
	Number exp = lev.pow_sum(player._get_AIM_effect() - 1)
	             * player._get_alpha_aero_effect_i();
	Number effect = ba_base.pow(exp);

	ImGui::Text("%s BA%d", N2S(player.upgrade[58 + i].level).c_str(), i + 1);
	ImGui::Text("Produce %s BM%d/s", N2S(effect).c_str(), i + 1);
	ImGui::Text("Cost: %s Bm",
	            N2S(player._level_cost(58 + i, player.upgrade[58 + i].level)).c_str());

	const char* a_label = g_bulk_buy
	                      ? (false ? "Max BA" : "Buy Max BA") // The false here is for space of future BA/BI Autobuyers.
		                      : "Buy BA";
	string a_btn = string(a_label) + to_string(i + 1);

	if(BetaButton(a_btn.c_str(), ImVec2(120, 0))) {
		if (g_bulk_buy) player._try_upgrade_bulk(i + 58);
		Number x = player.upgrade[i + 58].level;
		player._try_upgrade(i + 58);
		while(x != player.upgrade[i + 58].level) {
			x = player.upgrade[i + 58].level;
			player._try_upgrade(i + 58);
		}
	}
	ImGui::Separator();

	// Beta Intensifier
	ImGui::Text("%s BI%d", N2S(player.upgrade[66 + i].level).c_str(), i + 1);
	ImGui::Text("Empower BM/s to ^%s", N2S(exp).c_str());
	ImGui::Text("Cost: %s Bm",
	            N2S(player._level_cost(66 + i, player.upgrade[66 + i].level)).c_str());
	const char* i_label = g_bulk_buy
	                      ? (false ? "Max BI" : "Buy Max BI") // The false here is for space of future BA/BI Autobuyers.
		                      : "Buy BI";
	string i_btn = string(i_label) + to_string(i + 1);

	if(BetaButton(i_btn.c_str(), ImVec2(120, 0))) {
		if (g_bulk_buy) player._try_upgrade_bulk(i + 66);
		Number x = player.upgrade[i + 66].level;
		player._try_upgrade(i + 66);
		while(x != player.upgrade[i + 66].level) {
			x = player.upgrade[i + 66].level;
			player._try_upgrade(i + 66);
		}
	}

	ImGui::EndChild();
	ImGui::EndGroup();
}

int subtab = 1;

//Matter tab, the initial tab.
void DrawMatterTab(player_& player) {
	ImGui::SetNextWindowPos(ImVec2(0, 42));
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - 42));
	ImGui::Begin("##Matter", nullptr,
	             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
	             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

	ImGui::Text("Matter: %s  ( +%s/s", N2S(player.pre_alpha.score).c_str(), N2S(rate).c_str());
	if(player.unlocked_beta) {
		string positive = "";
		if(player.rate_Number > ZERO) positive = "+";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),"->%s%s/s", positive.c_str(), N2S(player.rate_Number).c_str());
	}
	ImGui::SameLine();
	ImGui::Text(")");

	if(player.unlocked_beta) {
		string s = "x";
		if(player.pre_alpha.score >= TEN.pow(TEN)) s = "^";
		if(player.pre_alpha.score >= TEN.pow(TEN.pow(TEN))) s = "^^";
		if(player.pre_alpha.score >= TEN.pow(TEN.pow(TEN.pow(TEN)))) s = "^^^";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),
		                   "Your Matters are decaying. Matter: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.pre_alpha.score, ONE)).c_str());
		if(player.upgrade[34].level.is_one()) {
			s = "x";
			if(player.alpha.score >= TEN.pow(TEN)) s = "^";
			if(player.alpha.score >= TEN.pow(TEN.pow(TEN))) s = "^^";
			if(player.alpha.score >= TEN.pow(TEN.pow(TEN.pow(TEN)))) s = "^^^";
			ImGui::SameLine();
			ImGui::TextColored(_get_BD_color(),
			                   " Am: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.alpha.score, ONE)).c_str());
		}
		s = "x";
		if(player.beta.score >= TEN.pow(TEN)) s = "^";
		if(player.beta.score >= TEN.pow(TEN.pow(TEN))) s = "^^";
		if(player.beta.score >= TEN.pow(TEN.pow(TEN.pow(TEN)))) s = "^^^";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),
		                   " Bm: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.beta.score, ONE)).c_str());
	}

	ImGui::Text("GIM = 10-logPF(log10(Prest.)) = %s -> ^%s to Mult, PIM: %s -> PF = %s.",
	            N2S(player._get_GIM()).c_str(), N2S(player._get_GIM_effect()).c_str(),
	            N2S(player._get_PIM()).c_str(), N2S(player._get_PF()).c_str());

	ImGui::Separator();

	if (ImGui::Button("Matter", ImVec2(200, 0))) {
		subtab = 1;
	}
	if(player.unlocked_alpha) {
		ImGui::SameLine();
		if (AlphaButton("Alpha##alphatab", ImVec2(200, 0))) subtab = 2;
	}
	if(player.unlocked_beta) {
		ImGui::SameLine();
		if (BetaButton("Beta##betatab", ImVec2(200, 0))) subtab = 3;
	}
	ImGui::Separator();
	if(subtab == 1) {
		ImGui::Text("Matter: %s  ( +%s/s = (M1 * M2 *...* M8) ^ %s",
		            N2S(player.pre_alpha.score).c_str(),
		            N2S(rate).c_str(),
		            N2S(player._get_GIM_effect()).c_str());
		ImGui::SameLine();
		if(player.unlocked_beta) {
			ImGui::TextColored(_get_BD_color(), "->%s/s", N2S(player.rate_Number).c_str());
			ImGui::SameLine();
		}
		ImGui::Text(")");
		ImGui::Text("GIM = %s -> ^%s to Mult.",
		            N2S(player._get_GIM()).c_str(),
		            N2S(player._get_GIM_effect()).c_str());
		ImGui::Text("PIM: %s -> PF = %s.",
		            N2S(player._get_PIM()).c_str(),
		            N2S(player._get_PF()).c_str());
		ImGui::Separator();

		for (int i = 0; i < 8; ++ i) {
			DrawUpgradeCard(player, i);
		}
		if (ImGui::Button("Max All (M)", ImVec2(200, 0)))
			player._try_upgrade_bulk_all();

		// A/I buy mode switch
		ImGui::SameLine();
		if (g_bulk_buy) {
			if (ImGui::Button("A/I Mode: Max (Click to Single)", ImVec2(280, 0))) {
				g_bulk_buy = false;
			}
		} else {
			if (ImGui::Button("A/I Mode: Single (Click to Max)", ImVec2(280, 0))) {
				g_bulk_buy = true;
			}
		}
	} else if (subtab == 2) {
		if (!player.upgrade[34].level.is_one()) {
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f),
			                   "AA/AI system locked. Purchase BAU5 under Alpha > Break Alpha Upgrades.");
		} else {
			ImGui::Text("Alpha Matter (Am): %s  ( +%s/s = (M1 * M2 *...* M8) ^ %s",
			            N2S(player.alpha.score).c_str(),
			            N2S(player.rate_Number_Am).c_str(),
			            N2S(player._get_AIM_effect()).c_str());
			ImGui::SameLine();
			if(player.unlocked_beta) {
				ImGui::TextColored(_get_BD_color(), "->%s/s", N2S(player.rate_Number_Am).c_str());
				ImGui::SameLine();
			}
			ImGui::Text(")");
			ImGui::Text("AIM = %s -> ^%s to Alpha Mult.",
			            N2S(player._get_AIM()).c_str(),
			            N2S(player._get_AIM_effect()).c_str());
			ImGui::Text("Am effects: -GIM x%s, -PIM x%s, Amp. x%s, Int. x%s",
			            N2S(player._get_Am_effect(1)).c_str(),
			            N2S(player._get_Am_effect(2)).c_str(),
			            N2S(player._get_Am_effect(3)).c_str(),
			            N2S(player._get_Am_effect(4)).c_str());
			ImGui::Separator();

			for (int i = 0; i < 8; ++i) {
				DrawAlphaUpgradeCard(player, i);
			}

			ImGui::Separator();
			if (AlphaButton("Max All AA & AI", ImVec2(240, 0))) {
				for (int i = 0; i < 8; ++i) {
					player._try_upgrade_bulk(40 + i);
					player._try_upgrade_bulk(48 + i);
					player._try_upgrade(40 + i);
					player._try_upgrade(48 + i);
				}
			}

			ImGui::SameLine();
			if (g_bulk_buy) {
				if (AlphaButton("AA/AI Mode: Max (Click to Single)", ImVec2(280, 0))) {
					g_bulk_buy = false;
				}
			} else {
				if (AlphaButton("AA/AI Mode: Single (Click to Max)", ImVec2(280, 0))) {
					g_bulk_buy = true;
				}
			}
		}
	} else if (subtab == 3) {
		ImGui::Text("Beta Matter (Bm): %s  ( +%s/s = (M1 * M2 *...* M8) ^ %s",
		            N2S(player.alpha.score).c_str(),
		            N2S(player.rate_Number_Bm).c_str(),
		            N2S(player._get_BIM_effect()).c_str());
		ImGui::SameLine();
		if(player.unlocked_beta) {
			ImGui::TextColored(_get_BD_color(), "->%s/s", N2S(player.rate_Number_Bm).c_str());
			ImGui::SameLine();
		}
		ImGui::Text(")");
		ImGui::Text("BIM = %s -> ^%s to Beta Mult.",
		            N2S(player._get_BIM()).c_str(),
		            N2S(player._get_BIM_effect()).c_str());
		ImGui::Text("Bm effects: Narrow BDF, BDN, empower Am effect by %s, x%s to Alpha, x%s to Beta.",
		            N2S(player._get_Bm_effect(1)).c_str(),
		            N2S(player._get_Bm_effect(4)).c_str(),
		            N2S(player._get_Bm_effect(5)).c_str());
		ImGui::Separator();

		for (int i = 0; i < 8; ++i) {
			DrawBetaUpgradeCard(player, i);
		}

		ImGui::Separator();
		if (BetaButton("Max All BA & BI", ImVec2(240, 0))) {
			for (int i = 0; i < 8; ++i) {
				player._try_upgrade_bulk(58 + i);
				player._try_upgrade_bulk(66 + i);
				player._try_upgrade(58 + i);
				player._try_upgrade(66 + i);
			}
		}

		ImGui::SameLine();
		if (g_bulk_buy) {
			if (BetaButton("BA/BI Mode: Max (Click to Single)", ImVec2(280, 0))) {
				g_bulk_buy = false;
			}
		} else {
			if (BetaButton("BA/BI Mode: Single (Click to Max)", ImVec2(280, 0))) {
				g_bulk_buy = true;
			}
		}
	}

	ImGui::Separator();

	// Prestige line.
	if(player.unlocked_prestige) {
		if(player.pre_alpha.highest >= player.pre_alpha.score) {
			string s = " (Resets A1-A8, I1-I8)";
			if(player.upgrade[16].level.is_one()) s = " (Resets A1-A8, I1-I8 and Aeromagnifiers)";
			if(player._get_AM(0))  s = "";
			ImGui::Text("Prestige%s at more than %s matter to gain a further boost.",
			            s.c_str(),
			            N2S(player.pre_alpha.highest).c_str());
		} else {
			string s = " (Resets A1-A8, I1-I8)";
			if(player._get_AM(0))  s = "";
			if(player.upgrade[16].level.is_one()) s = " (Resets A1-A8, I1-I8 and Aeromagnifiers)";
			string s1 = "";
			if(player._get_ach(32)) s1 = ", and is giving Mult. to Amp. by x" + N2S(player._get_PU2_effect(player.pre_alpha.highest));

			ImGui::Text("Prestige%s at %s matter to reduce GIM to %s%s.",
			            s.c_str(),
			            N2S(player.pre_alpha.score).c_str(),
			            N2S(player._get_GIM(player.pre_alpha.score)).c_str(),
			            s1.c_str());
			ImGui::SameLine();
			if(ImGui::Button(("Prestige"))) init_prestige(player);
		}
	} else {
		if(player.upgrade[2].level.is_zero()) ImGui::Text("Obtain at least an A3 to unlock Prestige.");
		else {
			string s = " (Resets A1-A8, I1-I8)";
			if(player.upgrade[16].level.is_one()) s = " (Resets A1-A8, I1-I8 and Aeromagnifiers)";
			if(player._get_AM(0))  s = "";
			string s1 = "";
			if(player._get_ach(32)) s1 = ", and mults Amp. by x" + N2S(player._get_PU2_effect(player.pre_alpha.highest));

			ImGui::Text("Prestige%s at %s matter to reduce GIM to %s%s.",
			            s.c_str(),
			            N2S(player.pre_alpha.score).c_str(),
			            N2S(player._get_GIM(player.pre_alpha.score)).c_str(),
			            s1.c_str());
			ImGui::SameLine();
			if(ImGui::Button(("Prestige"))) init_prestige(player);
		}
	}

	// Alpha line.
	if(player.unlocked_alpha || player.pre_alpha.score >= inf) {
		Number s = player.pre_alpha.score;
		if(player.pre_alpha.score < inf) s = inf;
		ImGui::Text("Reach Alpha at %s matter to gain %s Alpha Points and %s Alpha.",
		            N2S(s).c_str(),
		            N2S(player._get_AP()).c_str(),
		            N2S(player._get_alpha()).c_str());
		if(player.pre_alpha.score >= inf) {
			ImGui::SameLine();
			if(ImGui::Button(("Alpha"))) init_alpha(player);
		}
	}

	// Beta line.
	if(player.unlocked_beta || player.alpha.score >= inf) {
		Number s = player.alpha.score;
		if(player.alpha.score < inf) s = inf;
		ImGui::Text("Reach Beta at %s Alpha matter to gain %s Beta Points and %s Beta.",
		            N2S(s).c_str(),
		            N2S(player._get_BP()).c_str(),
		            N2S(player._get_beta()).c_str());
		if(player.alpha.score >= inf) {
			ImGui::SameLine();
			if(ImGui::Button(("Beta"))) init_beta(player);
		}
	}

	ImGui::Separator();


	// Aeromagnifiers.
	if (player.upgrade[16].level.is_one() && subtab == 1) {
		ImGui::Separator();
		ImGui::Text("Aeromagnifiers:");
		for (int id = 20; id <= 21; ++id) {
			Number l = player.upgrade[id].level;
			string s = " (Resets A1-A8, I1-I8)";
			if(player._get_AM(0)) s = ""; //First Alpha Milestone: Aero no longer resets.
			ImGui::Text("%s %s  Cost: %s%s",
			            N2S(l).c_str(), (id == 20 ? "Amp. Aeromagnifier" : "Int. Aeromagnifier"),
			            N2S(player._level_cost(id, player.upgrade[id].level)).c_str(),
			            s.c_str());
			ImGui::SameLine();
			if (ImGui::Button(("Buy Max##aero" + to_string(id)).c_str())) {
				bool ok = (player._level_cost(id, player.upgrade[id].level) <= player.pre_alpha.score);
				player._try_upgrade(id);
				player._try_upgrade_bulk(id);
				if(ok && !player._get_AM(0)) init_aero(player);
			}
			if(player._get_AM(5)) {
				ImGui::SameLine();
				string at = (player.upgrade[id].autobuyer.enabled) ? "Auto: ON" : "Auto: OFF";
				if(ImGui::Button((at + "##autoaero" + to_string(id)).c_str())) player.upgrade[id].autobuyer.enabled = !player.upgrade[id].autobuyer.enabled;
			}
		}
	}
	// Alpha Aeros.
	if (player.upgrade[35].level.is_one() && subtab == 2) {
		ImGui::Separator();
		ImGui::Text("Alpha Aeromagnifiers:");
		for (int id = 56; id <= 57; ++id) {
			Number l = player.upgrade[id].level;
			string s = " (Resets AA1-A8, AI1-I8)";
			ImGui::Text("%s %s  Cost: %s%s",
			            N2S(l).c_str(), (id == 56 ? "Alpha Amp. Aeromagnifier" : "Alpha Int. Aeromagnifier"),
			            N2S(player._level_cost(id, player.upgrade[id].level)).c_str(),
			            s.c_str());
			ImGui::SameLine();
			if (AlphaButton(("Buy Max##alphaaero" + to_string(id)).c_str())) {
				bool ok = (player._level_cost(id, player.upgrade[id].level) <= player.alpha.score);
				player._try_upgrade_bulk(id);
				player._try_upgrade(id);
				if(ok) init_alpha_aero(player);
			}
		}
	}

	ImGui::End();
}

static std::string get_timestamp();
static std::string get_save_string();
static void load_game_state_from_string(const std::string& data);
void save_game_state(player_& player);
void load_game_state(const string& filename, player_& player);
static std::string paste_from_clipboard();
#ifdef _WIN32
static void copy_to_clipboard(const std::string& text);
#endif

int optionstab = 1;
//Options tab.
void DrawOptionsTab(player_& player) {
	ImGui::SetNextWindowPos(ImVec2(0, 42));
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - 42));
	ImGui::Begin("##Options", nullptr,
	             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
	             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);


	if (ImGui::Button("Options", ImVec2(200, 0))) {
		optionstab = 1;
	}
	if(player.unlocked_alpha) {
		ImGui::SameLine();
		if (ImGui::Button("Automation", ImVec2(200, 0))) optionstab = 2;
	}

	if(optionstab == 1) {
		ImGui::Text("Time Flux: %s", _to_time(player.time_flux).c_str());
		if (!flux_pool.is_zero() && flux_elapsed < FLUX_DURATION) {
			long double accel = 1.0L + flux_pool._to_long_double() / (FLUX_DURATION - flux_elapsed);
			ImGui::TextColored(ImVec4(1,1,0.3f,1), "Accelerating x%.2Lf (%.1Lfs left)",
			                   accel, FLUX_DURATION - flux_elapsed);
		}
		ImGui::Separator();

		// Time Flux Mode
		static int flux_mode = 0;
		flux_mode = (int)player.options[0].val._to_long_double();
		ImGui::Text("Spend Time Flux:");
		ImGui::RadioButton("Seconds", &flux_mode, 0);
		ImGui::SameLine();
		static float sec_amount = 60.0f;
		sec_amount = (float)player.options[1].val._to_long_double();
		if (ImGui::DragFloat("##sec", &sec_amount, 1.0f, 0.0f, 3600.0f, "%.1f s"))
			player.options[1].val = (long double)sec_amount;

		ImGui::RadioButton("Percent", &flux_mode, 1);
		ImGui::SameLine();
		static float pct_amount = 10.0f;
		pct_amount = (float)player.options[2].val._to_long_double();
		if (ImGui::DragFloat("##pct", &pct_amount, 1.0f, 0.0f, 100.0f, "%.1f %%"))
			player.options[2].val = (long double)pct_amount;
		player.options[0].val = flux_mode;

		if (ImGui::Button("Activate (Shift+Enter)", ImVec2(240, 0))) {
			long double amt = (flux_mode == 0) ? sec_amount
			                  : player.time_flux * pct_amount / 100.0L;
			if (amt > player.time_flux) amt = player.time_flux;
			if (amt > 0) {
				player.time_flux -= amt;
				flux_pool = amt;
				flux_elapsed = 0;
			}
		}

		//Savefile part.
		ImGui::Separator();
		ImGui::Text("Save file:");
		static char save_buf[256];
		if (!ImGui::IsAnyItemActive())
			strncpy(save_buf, player.options[3].val_s.c_str(), sizeof(save_buf) - 1);
		save_buf[sizeof(save_buf) - 1] = '\0';
		if (ImGui::InputText("##save", save_buf, sizeof(save_buf), ImGuiInputTextFlags_EnterReturnsTrue))
			player.options[3].val_s = save_buf;

		//BGM part.
		ImGui::Separator();
		ImGui::Text("BGM:");
		static int song_index = 0;
		song_index = (int)player.options[4].val._to_long_double();
		vector<string> name_store;
		vector<const char*> name_ptrs;
		for (auto& s : BGM_LIST) {
			string n = s;
			if (n.size() > 4 && n.substr(n.size() - 4) == ".mp3") n = n.substr(0, n.size() - 4);
			name_store.push_back(n);
		}
		for (auto& s : name_store) name_ptrs.push_back(s.c_str());
		if (ImGui::Combo("Song", &song_index, name_ptrs.data(), (int)name_ptrs.size()))
			player.options[4].val = song_index;

		static float volume = 50.0f;
		volume = (float)player.options[5].val._to_long_double();
		if (ImGui::SliderFloat("Volume", &volume, 0.0f, 100.0f, "%.0f%%"))
			player.options[5].val = (long double)volume;

		// Save Management: Export / Import
		ImGui::Separator();
		ImGui::Text("Save Management:");

		// --- Export Save ---
		if (ImGui::Button("Export Save", ImVec2(200, 0))) {
			std::string save_str = get_save_string();
			if (save_str.empty()) {
				PushNotification("Export Failed",
				                 "Could not serialize save.",
				                 ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
			} else {
#ifdef _WIN32
				copy_to_clipboard(save_str);
#endif
				PushNotification("Exported Save",
				                 "Save data copied to clipboard.",
				                 ImVec4(0.35f, 1.0f, 0.45f, 1.0f));
			}
		}
		ImGui::SameLine();
		// --- Export Save as File ---
		if (ImGui::Button("Export Save as File", ImVec2(220, 0))) {
			std::string save_str = get_save_string();
			std::string fname = "save-" + get_timestamp() + ".txt";
			if (save_str.empty()) {
				PushNotification("Export Failed",
				                 "Could not serialize save.",
				                 ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
			} else {
				std::ofstream f(fname, std::ios::binary);
				if (!f) {
					PushNotification("Export Failed",
					                 "Could not write " + fname,
					                 ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
				} else {
					f << save_str;
					f.close();
					PushNotification("Exported Save",
					                 "Saved as " + fname,
					                 ImVec4(0.35f, 1.0f, 0.45f, 1.0f));
				}
			}
		}

		// --- Import Save ---
		ImGui::Separator();
		ImGui::Text("Import Save (paste below; will NOT save current game):");
		static char import_buf[1 << 16] = "";
		ImGui::InputTextMultiline("##importdata", import_buf, sizeof(import_buf),
		                          ImVec2(-1, 100), ImGuiInputTextFlags_AllowTabInput);
		if (ImGui::Button("Import Save", ImVec2(200, 0))) {
			if (import_buf[0] != '\0') {
				init(player);
				load_game_state_from_string(std::string(import_buf));
				PushNotification("Imported Save",
				                 "Save loaded into memory (disk unchanged).",
				                 ImVec4(0.55f, 0.85f, 1.0f, 1.0f));
			} else {
				PushNotification("Import Skipped",
				                 "Input buffer is empty.",
				                 ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Paste from Clipboard", ImVec2(200, 0))) {
#ifdef _WIN32
			std::string clip = paste_from_clipboard();
			if (clip.empty()) {
				PushNotification("Paste Failed",
				                 "Clipboard is empty or unavailable.",
				                 ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
			} else {
				std::strncpy(import_buf, clip.c_str(), sizeof(import_buf) - 1);
				import_buf[sizeof(import_buf) - 1] = '\0';
			}
#endif
		}

		// --- Import Save from File ---
		ImGui::Separator();
		ImGui::Text("Import Save from File (current save is backed up first):");
		static char import_file_buf[256] = "save1.txt";
		ImGui::InputText("##importfile", import_file_buf, sizeof(import_file_buf));
		if (ImGui::Button("Import from File", ImVec2(200, 0))) {
			std::string target = import_file_buf;

			std::ifstream test(target, std::ios::binary);
			if (!test.good()) {
				PushNotification("Import Skipped",
				                 "File not found: " + target,
				                 ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
			} else {
				test.close();

				std::string cur = get_save_string();
				std::string backup = "save-" + get_timestamp() + ".txt";
				if (!cur.empty()) {
					std::ofstream bf(backup, std::ios::binary);
					if (bf) {
						bf << cur;
						bf.close();
					}
				}
				load_game_state(target, player);
				PushNotification("Imported Save",
				                 "Loaded from " + target + "\nBackup: " + backup,
				                 ImVec4(0.55f, 0.85f, 1.0f, 1.0f));
			}
		}
	}

	//Autobuyer tab.
	if(optionstab == 2) {
		ImGui::BeginChild("##autotab", ImVec2(-1, 700), true);
		//Amplifier autobuyer.
		if(player._get_AM(1)) {
			for(int i = 0; i < 8; i ++) {
				string en = (player.upgrade[i].autobuyer.enabled) ? "Auto: ON" : "Auto: OFF";
				ImGui::BeginChild(("##atba" + to_string(i)).c_str(), ImVec2(300, 80), true);
				ImGui::Text("Amp. %s autobuyer:", to_string(i + 1).c_str());
				if(ImGui::Button(en.c_str())) {
					player.upgrade[i].autobuyer.enabled ^= 1;
				}
				ImGui::EndChild();
				if(i % 4 != 3) ImGui::SameLine();
			}
		}

		//Intensifier autobuyer.
		if(player._get_AM(2)) {
			for(int i = 0; i < 8; i ++) {
				string en = (player.upgrade[i + 8].autobuyer.enabled) ? "Auto: ON" : "Auto: OFF";
				ImGui::BeginChild(("##atbi" + to_string(i)).c_str(), ImVec2(300, 80), true);
				ImGui::Text("Int. %s autobuyer:", to_string(i + 1).c_str());
				if(ImGui::Button(en.c_str())) {
					player.upgrade[i + 8].autobuyer.enabled ^= 1;
				}
				ImGui::EndChild();
				if(i % 4 != 3) ImGui::SameLine();
			}
		}

		//Aeromagnifier autobuyer.
		if(player._get_AM(5)) {
			for(int id = 20; id <= 21; id ++) {
				string en = (player.upgrade[id].autobuyer.enabled) ? "Auto: ON" : "Auto: OFF";
				string nm = (id == 20) ? "Amp. Aero. Autobuyer" : "Int. Aero. Autobuyer";
				ImGui::BeginChild(("##atbaero" + to_string(id)).c_str(), ImVec2(600, 80), true);
				ImGui::Text(nm.c_str());
				if(ImGui::Button(en.c_str())) {
					player.upgrade[id].autobuyer.enabled ^= 1;
				}
				ImGui::EndChild();
				if(id == 20) ImGui::SameLine();
			}
		}

		//Alpha autobuyer.
		if(player._get_AM(6)) {
			string en = (player.alpha.autobuyer) ? "Auto: ON" : "Auto: OFF";
			string nm = "Alpha Autobuyer";
			ImGui::BeginChild("##atbalpha" , ImVec2(-1, 80), true);
			ImGui::Text(nm.c_str());
			if(ImGui::Button(en.c_str())) {
				player.alpha.autobuyer ^= 1;
			}
			ImGui::EndChild();
			ImGui::SameLine();
		}
		ImGui::EndChild();
	}


	ImGui::End();
}

//Prestige tab.
void DrawPrestigeTab(player_& player) {
	ImGui::SetNextWindowPos(ImVec2(0, 42));
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - 42));
	ImGui::Begin("##Prestige", nullptr,
	             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
	             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

	ImGui::Text("Matter: %s  ( +%s/s", N2S(player.pre_alpha.score).c_str(), N2S(rate).c_str());
	if(player.unlocked_beta) {
		string positive = "";
		if(player.rate_Number > ZERO) positive = "+";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),"->%s%s/s", positive.c_str(), N2S(player.rate_Number).c_str());
	}
	ImGui::SameLine();
	ImGui::Text(")");

	if(player.unlocked_beta) {
		string s = "x";
		if(player.pre_alpha.score >= TEN.pow(TEN)) s = "^";
		if(player.pre_alpha.score >= TEN.pow(TEN.pow(TEN))) s = "^^";
		if(player.pre_alpha.score >= TEN.pow(TEN.pow(TEN.pow(TEN)))) s = "^^^";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),
		                   "Your Matters are decaying. Matter: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.pre_alpha.score, ONE)).c_str());
		if(player.upgrade[34].level.is_one()) {
			s = "x";
			if(player.alpha.score >= TEN.pow(TEN)) s = "^";
			if(player.alpha.score >= TEN.pow(TEN.pow(TEN))) s = "^^";
			if(player.alpha.score >= TEN.pow(TEN.pow(TEN.pow(TEN)))) s = "^^^";
			ImGui::SameLine();
			ImGui::TextColored(_get_BD_color(),
			                   " Am: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.alpha.score, ONE)).c_str());
		}
		s = "x";
		if(player.beta.score >= TEN.pow(TEN)) s = "^";
		if(player.beta.score >= TEN.pow(TEN.pow(TEN))) s = "^^";
		if(player.beta.score >= TEN.pow(TEN.pow(TEN.pow(TEN)))) s = "^^^";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),
		                   " Bm: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.beta.score, ONE)).c_str());
	}

	ImGui::Text("GIM = 10-logPF(log10(Prest.)) = %s -> ^%s to Mult, PIM: %s -> PF = %s.",
	            N2S(player._get_GIM()).c_str(), N2S(player._get_GIM_effect()).c_str(),
	            N2S(player._get_PIM()).c_str(), N2S(player._get_PF()).c_str());

	ImGui::Separator();

	const char* pu_names[4] = {
		"(PU1) Unlock Aeromagnifiers",
		"(PU2) Give a mult. to M/s before Int. effect",
		"(PU3) Reduce PIM by 10% GIM reduction",
		"(PU4) Start with 10000 matter and strengthen Amp. Aeros and Int. Aeros."
	};
	const char* pu_cost[4] = { "1e16", "1e30", "1e50", "1e100" };
	for (int i = 0; i < 4; ++i) {
		ImGui::BeginChild(("##pu" + to_string(i)).c_str(), ImVec2(0, 70), true);
		ImGui::Text("%s", pu_names[i]);
		if (player.upgrade[i + 16].level.is_one()) {
			string s;
			if(i == 0 || i == 3) s = "Enabled";
			else if(i == 1) s = "x" + player._get_PU2_effect().toString();
			else if(i == 2) s = "-" + player._get_PU3_effect().toString();
			ImGui::TextColored(ImVec4(0.3f,1,0.3f,1), "Currently: %s", s.c_str());
		}

		else {
			ImGui::Text("Cost: %s matter", pu_cost[i]);
			if (ImGui::Button(("Buy PU" + to_string(i + 1)).c_str()))
				player._try_upgrade(i + 16);
		}
		ImGui::EndChild();
	}

	ImGui::Separator();

	// Prestige line.
	if(player.unlocked_prestige) {
		if(player.pre_alpha.highest >= player.pre_alpha.score) {
			string s = " (Resets A1-A8, I1-I8)";
			if(player.upgrade[16].level.is_one()) s = " (Resets A1-A8, I1-I8 and Aeromagnifiers)";
			if(player._get_AM(0))  s = "";
			ImGui::Text("Prestige%s at more than %s matter to gain a further boost.",
			            s.c_str(),
			            N2S(player.pre_alpha.highest).c_str());
		} else {
			string s = " (Resets A1-A8, I1-I8)";
			if(player._get_AM(0))  s = "";
			if(player.upgrade[16].level.is_one()) s = " (Resets A1-A8, I1-I8 and Aeromagnifiers)";
			string s1 = "";
			if(player._get_ach(32)) s1 = ", and is giving Mult. to Amp. by x" + N2S(player._get_PU2_effect(player.pre_alpha.highest));

			ImGui::Text("Prestige%s at %s matter to reduce GIM to %s%s.",
			            s.c_str(),
			            N2S(player.pre_alpha.score).c_str(),
			            N2S(player._get_GIM(player.pre_alpha.score)).c_str(),
			            s1.c_str());
			ImGui::SameLine();
			if(ImGui::Button(("Prestige"))) init_prestige(player);
		}
	} else {
		if(player.upgrade[2].level.is_zero()) ImGui::Text("Obtain at least an A3 to unlock Prestige.");
		else {
			string s = " (Resets A1-A8, I1-I8)";
			if(player.upgrade[16].level.is_one()) s = " (Resets A1-A8, I1-I8 and Aeromagnifiers)";
			if(player._get_AM(0))  s = "";
			string s1 = "";
			if(player._get_ach(32)) s1 = ", and mults Amp. by x" + N2S(player._get_PU2_effect(player.pre_alpha.highest));

			ImGui::Text("Prestige%s at %s matter to reduce GIM to %s%s.",
			            s.c_str(),
			            N2S(player.pre_alpha.score).c_str(),
			            N2S(player._get_GIM(player.pre_alpha.score)).c_str(),
			            s1.c_str());
			ImGui::SameLine();
			if(ImGui::Button(("Prestige"))) init_prestige(player);
		}
	}

	// Alpha line.
	if(player.unlocked_alpha || player.pre_alpha.score >= inf) {
		Number s = player.pre_alpha.score;
		if(player.pre_alpha.score < inf) s = inf;
		ImGui::Text("Reach Alpha at %s matter to gain %s Alpha Points and %s Alpha.",
		            N2S(s).c_str(),
		            N2S(player._get_AP()).c_str(),
		            N2S(player._get_alpha()).c_str());
		if(player.pre_alpha.score >= inf) {
			ImGui::SameLine();
			if(ImGui::Button(("Alpha"))) init_alpha(player);
		}
	}

	// Beta line.
	if(player.unlocked_beta || player.alpha.score >= inf) {
		Number s = player.alpha.score;
		if(player.alpha.score < inf) s = inf;
		ImGui::Text("Reach Beta at %s Alpha matter to gain %s Beta Points and %s Beta.",
		            N2S(s).c_str(),
		            N2S(player._get_BP()).c_str(),
		            N2S(player._get_beta()).c_str());
		if(player.alpha.score >= inf) {
			ImGui::SameLine();
			if(ImGui::Button(("Beta"))) init_beta(player);
		}
	}


	ImGui::End();
}

// --------------------------------------------------------------------
// sim_safe_px(): clamp any (possibly huge / NaN / inf) coordinate coming
// from the big-number type into a finite pixel value.
// A single non-finite vertex would otherwise poison the whole ImDrawList
// batch and make the entire board (frame included) disappear.
// --------------------------------------------------------------------
static float sim_safe_px(long double v, float lo, float hi) {
	if (!std::isfinite((double)v)) return lo;
	double d = (double)v;
	if (d < (double)lo) return lo;
	if (d > (double)hi) return hi;
	return (float)d;
}

void DrawAlphaSimulator(player_& player) {
	simulator_& sim = player.alpha.simulator;
// ---- Effect members driven by SL / Achievements (set every frame). ----
// These are read back by _get_val / _get_gravity / _get_init_speed.
//   Global level bonus  = SL level  (+5 if Ach.47 unlocked). Shown in "Base(+bonus)".
//   Drop value multiplier = x1.10 when Ach.44 is unlocked.
	{
		Number _gb = sim.level;
		if (player._get_ach(47)) _gb = _gb + Number(5);
		sim.global_level_bonus = _gb;
		sim.drop_value_mult = player._get_ach(42) ? Number(1.1) : ONE; // Ach.44: +10% Drop value
	}
// ---- Part 0: header line (full width) ----
	ImGui::Text("You have %s Simulation Points (SP) and you're at %s Simulation Level (SL).",
	            N2S(sim.score).c_str(),
	            N2S(sim.level).c_str());
    ImGui::Text("They translated to a x%s to AP, a x%s to SP, and a x%s to Am generation.",
				N2S(player._get_SP_effect(1)).c_str(),
				N2S(player._get_SP_effect(3)).c_str(),
	            N2S(player._get_SP_effect(2)).c_str());
	ImGui::Separator();
// ---- SL progress bar (full width, sits ABOVE the board) ----
	Number cost     = sim._get_cost();
	float  progress = 0.0f;
	if (cost > ZERO) {
		long double pr = (sim.accumulate / cost)._to_long_double();
		if (!std::isfinite((double)pr)) pr = 0.0L;
		if (pr < 0) pr = 0;
		if (pr > 1) pr = 1;
		progress = (float)pr;
	}
	ImGui::ProgressBar(progress, ImVec2(-1, 0),
	                   (std::string("SL ") + N2S(sim.level) + " -> " + N2S(sim.level + ONE) + " : "
	                     + N2S(sim.accumulate) + " / " + N2S(cost) + " SP").c_str());
	// ---- Split the rest into LEFT (board) and RIGHT (upgrades). ----
	const float CARD_H   = 200.0f;   // one tier card
	ImVec2 avail = ImGui::GetContentRegionAvail();
	float total_h = avail.y;
	if (total_h < 240.0f) total_h = 240.0f;
	float col_gap = 12.0f;
	float left_w  = (avail.x - col_gap) * 0.60f;
	if (left_w  < 300.0f) left_w  = 300.0f;
	float right_w = (avail.x - col_gap) * 0.40f;
	if (right_w < 340.0f) right_w = 340.0f;
	// ============================ LEFT: the board ============================
	ImGui::BeginChild("##boardArea", ImVec2(left_w, total_h), true);
	{
		ImVec2 a1 = ImGui::GetContentRegionAvail();
		
		float bw = a1.x;
		if (bw  < 120.0f) bw  = 120.0f;

		float bh = a1.y;
		if (bh  < 160.0f) bh  = 160.0f;
		// Recycle the whole drop pool whenever the board pixel size changes (window
		// resize / layout shift) so every sprite re-seeds to the new extent.
		{
			int _bpw = (int)bw, _bph = (int)bh;
			if (_bpw != sim.board_w_px || _bph != sim.board_h_px) {
				sim.board_w_px = _bpw; sim.board_h_px = _bph; sim.drop_pool_dirty = true;
			}
		}

		ImGui::Dummy(ImVec2(bw, bh));
		const double VW = 800.0, VH = 800.0;

		ImDrawList* dl = ImGui::GetForegroundDrawList();

		ImVec2 cur = ImGui::GetCursorScreenPos();
		// (Dummy already advanced the cursor; recompute top-left from window origin.)

		cur = ImVec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y - bh);

		dl->AddRectFilled(cur, ImVec2(cur.x + bw, cur.y + bh),
		                  ImColor(0.08f, 0.08f, 0.12f, 1.0f));
		dl->AddRect(cur, ImVec2(cur.x + bw, cur.y + bh),
		            ImColor(0.30f, 0.45f, 0.85f, 1.0f));
		// Guaranteed ball seed safety net (board never blank if update() skipped a frame).
		{
			int na = 0;
			for (int i = 0; i < 10; i++) {
				long double v = sim.ball[i].value._to_long_double();
				if (std::isfinite((double)v) && v > 0.0L) na++;
			}
			if (na == 0) {
				for (int i = 0; i < 10; i++) {
					sim.ball[i].value = Number(10) * Number(2).pow(sim.level);
					sim.ball[i].spawn = sim.ball[i].value;
					sim.ball[i].x = Number((double)(30 + rand() % 740));
					sim.ball[i].y = Number((double)(30 + rand() % 740));
				}
			}
		}
		float sx = bw / (float)VW, sy = bh / (float)VH;

		float sc = (sx < sy) ? sx : sy;
		if (sc <= 0.0f) sc = 1.0f;

		if (sx <= 0.0f) sx = sc;
		if (sy <= 0.0f) sy = sc;

		float ox = cur.x, oy = cur.y;
		// Balls (fill whole left rect, per-axis scale, round via sc).
		int    N_BALLS   = 10;               // balls kept alive on the board
		if(sim._get_ach(45)) N_BALLS = 25;
		if(sim._get_ach(49)) N_BALLS = 50;

		float ball_r = 30.0f * sc;
		if (ball_r < 1.0f) ball_r = 1.0f;

		for (int i = 0; i < N_BALLS; i++) {
			long double vchk = sim.ball[i].value._to_long_double();
			if (!(std::isfinite((double)vchk) && vchk > 0.0L)) continue;
			long double bx = sim.ball[i].x._to_long_double();
			long double by = sim.ball[i].y._to_long_double();
			if (!std::isfinite((double)bx) || !std::isfinite((double)by)) continue;
			
			long double bx_ratio = 1.0L;
			if (sim.ball[i].spawn > ZERO) bx_ratio = (sim.ball[i].value / sim.ball[i].spawn)._to_long_double();

			if (!std::isfinite((double)bx_ratio)) bx_ratio = 1.0L;

			if (bx_ratio < 0) bx_ratio = 0;
			if (bx_ratio > 1) bx_ratio = 1;

			float br, bg, bb;

			ImGui::ColorConvertHSVtoRGB((float)(80.0 / 359.0 * bx_ratio), 240.0f / 255.0f, 160.0f / 255.0f, br, bg, bb);
			float px = ox + (float)bx * sx;
			float py = oy + (float)by * sy;
			if (px < cur.x - 64.0f || px > cur.x + bw + 64.0f) continue;
			if (py < cur.y - 64.0f || py > cur.y + bh + 64.0f) continue;
			dl->AddCircleFilled(ImVec2(px, py), ball_r, ImColor(br, bg, bb, 1.0f));
			{
				std::string val_str = sim.ball[i].value._to_String_short();
				bool pf = (g_font_small != nullptr);
				if (pf) ImGui::PushFont(g_font_small);
				ImVec2 ts = ImGui::CalcTextSize(val_str.c_str());
				dl->AddText(ImVec2(px - ts.x * 0.5f, py - ts.y * 0.5f), ImColor(1.0f, 1.0f, 1.0f, 1.0f), val_str.c_str());
				if (pf) ImGui::PopFont();
			}
		}
		// Drops: one dot per live sprite.
		static int DROP_HUE[10] = {122,  70,  43,  24,  10, 214, 189, 173, 147, 136};
		static int DROP_SAT[10] = {188, 183, 198, 198, 198, 198, 198, 198, 198, 198};
		static int DROP_LUM[10] = {240, 200, 200, 200, 200, 200, 200, 200, 200, 200};
		
		float drop_r = 10.0f * sc;
		if (drop_r < 1.0f) drop_r = 1.0f;

		for (int i = 0; i < simulator_::MAX_DROPS; i++) {
			if (sim.dtier[i] <= 0) continue;
			float px = ox + sim_safe_px(sim.dx[i], 0.0f, (float)VW) * sx;
			float py = oy + sim_safe_px(sim.dy[i], 0.0f, (float)VH) * sy;
			if (px < cur.x - 32.0f || px > cur.x + bw + 32.0f) continue;
			if (py < cur.y - 32.0f || py > cur.y + bh + 32.0f) continue;

			int   ti = sim.dtier[i];
			int   g  = (ti - 1) / 10;
			if (g < 0) g = 0;
			if (g > 9) g = 9;
			int   rem = ti - 1 - g * 10;
			float h   = DROP_HUE[g] / 240.0f;
			float s   = DROP_SAT[g] / 255.0f;
			float v   = (1.0f - 0.08f * rem) * DROP_LUM[g] / 240.0f;
			float dr, dg, db;
			ImGui::ColorConvertHSVtoRGB(h, s, v, dr, dg, db);
			
			float dalpha = sim._is_fast_drop(ti) ? 0.35f : 1.0f;   // ghost drops rendered semi-transparent
			dl->AddCircleFilled(ImVec2(px, py), drop_r, ImColor(dr, dg, db, dalpha));
		}
	}
	ImGui::EndChild();
	ImGui::SameLine();
	// ======================= RIGHT: the upgrades panel =======================
	ImGui::BeginChild("##upgradeArea", ImVec2(right_w, total_h), true);
	// ---- Top of the panel: the SL upgrade button ----
	{
		Number _sl_cost = sim._get_cost();
		bool _sl_ready = !(sim.accumulate < _sl_cost);
		ImGui::BeginDisabled(!_sl_ready);
		std::string _sl_btn = _sl_ready
		                      ? std::string("Upgrade SL  READY  [") + N2S(_sl_cost) + " Accumulated SP]"
		                      : std::string("Upgrade SL [Cost: ") + N2S(_sl_cost) + " Accumulated SP]";
		if (AlphaButton((_sl_btn + "##upgradeSL").c_str())) sim._try_upgrade();
		ImGui::EndDisabled();
		ImGui::Separator();
	}
    // Buy One / Buy Max toggle + current tier info.
	static bool bulk = false;
	if (AlphaButton(bulk ? "Mode: Buy Max" : "Mode: Buy One")) bulk = !bulk;
	ImGui::SameLine();
	int curTier = 1;
	for (int tt = 99; tt >= 1; tt--) {
		if (sim.drops[tt] > ZERO) {
			curTier = tt;
			break;
		}
	}
	ImGui::Text("Tier %d Drops: %s", curTier, N2S(sim.drops[curTier]).c_str());
	// ---- Tier upgrade list (2 cards per row, each upgrade on its OWN line) ----
	int hi = curTier + 1;
	if (hi > 99) hi = 99;
	int lo = hi - 5;
	if (lo < 1)   lo = 1;
	// Window sized to about TWO tier rows tall, rest scrolls.
	float tier_win_h = CARD_H * 2.0f + 24.0f;
	ImGui::BeginChild("##tier", ImVec2(-1, tier_win_h), true);
	// Preview helpers fold in the global bonus + Ach.44 multiplier so the
	// displayed "-> next" figures match the real _get_* result after buying.
	auto pv_val = [&](int t) -> Number {
		Number e = sim.level_global[0][0].level + sim.level_global[1][0].level + sim.global_level_bonus;
		Number a = Number(20).pow(t - 1) * (ONE + 0.05 * (sim.level_drops[t][0].level + ONE)) * Number(1.05).pow(e) * sim.drop_value_mult;
		if(sim._get_ach(42)) a = a * 1.5;
		if(sim._get_ach(50)) a = a * (sim.level + ONE);   // Ach.50: Drop value x (SL+1)
		return a;
	};
	auto pv_grav = [&](int t) -> Number {
		Number e = sim.level_global[0][1].level + sim.level_global[1][1].level + sim.global_level_bonus;
		Number a = (ONE + Number(0.05) * (sim.level_drops[t][1].level + ONE)) * Number(1.05).pow(e);
		if(sim._get_ach(44)) a = a * ((25 + sim.level).pow(0.5) - 4);
		if(sim._get_ach(46)) a = a * 2;
		return a;
	};
	auto pv_spd = [&](int t) -> Number {
		Number e = sim.level_global[0][1].level + sim.level_global[1][1].level + sim.global_level_bonus;
		Number a = (ONE + Number(0.05) * (sim.level_drops[t][2].level + ONE)) * Number(1.05).pow(e);
		if(sim._get_ach(42)) a = a * 1.5;
		return a;
	};
	for (int t = lo; t <= hi; t++) {
		ImGui::BeginChild(("##tier" + std::to_string(t)).c_str(), ImVec2(-1, CARD_H), true);
		ImGui::Text("Tier %d : %s", t, N2S(sim.drops[t]).c_str());
		// ---- Drop preview sprite (drawn on the RIGHT of this tier card) ----
		{
			ImDrawList* tdl = ImGui::GetWindowDrawList();
			ImVec2 wpos = ImGui::GetWindowPos();
			ImVec2 wsz  = ImGui::GetWindowSize();
			static int PREVIEW_HUE[10] = {122, 70, 43, 24, 10, 214, 189, 173, 147, 136};
			static int PREVIEW_SAT[10] = {188, 183, 198, 198, 198, 198, 198, 198, 198, 198};
			static int PREVIEW_LUM[10] = {240, 200, 200, 200, 200, 200, 200, 200, 200, 200};
			int pg = (t - 1) / 10; if (pg < 0) pg = 0; if (pg > 9) pg = 9;
			int rem = (t - 1) - pg * 10;
			float pvr, pvg, pvb;
			float h   = PREVIEW_HUE[pg] / 240.0f;
			float s   = PREVIEW_SAT[pg] / 255.0f;
			float v   = (1.0f - 0.08f * rem) * PREVIEW_LUM[pg] / 240.0f;
			ImGui::ColorConvertHSVtoRGB(h, s, v, pvr, pvg, pvb);
			float prev_r = 18.0f;
			ImVec2 pc(wpos.x + wsz.x - prev_r - 14.0f, wpos.y + prev_r + 12.0f);
			tdl->AddCircleFilled(pc, prev_r, ImColor(pvr, pvg, pvb, 1.0f));
			tdl->AddCircle(pc, prev_r, ImColor(1.0f, 1.0f, 1.0f, 0.7f), 0, 1.5f);
			std::string pl = std::string("T") + std::to_string(t);
			ImVec2 pls = ImGui::CalcTextSize(pl.c_str());
			tdl->AddText(ImVec2(pc.x - pls.x * 0.5f, pc.y - pls.y * 0.5f), ImColor(0.0f, 0.0f, 0.0f, 1.0f), pl.c_str());
		}
		const char* labels[3] = { "Value", "Gravity", "Init. speed" };
		Number curv[3];
		curv[0] = sim._get_val(t);
		curv[1] = sim._get_gravity(t);
		curv[2] = sim._get_init_speed(t);
		for (int k = 0; k < 3; k++) {
			Number up = (k == 0) ? pv_val(t) : (k == 1 ? pv_grav(t) : pv_spd(t));
			Number cost_k = sim._get_cost(t, k);
			ImGui::Text("%s: %s -> %s", labels[k], N2S(curv[k]).c_str(), N2S(up).c_str());
			string btn = std::string("[Cost: ") + N2S(cost_k) + "]";
			if (AlphaButton((btn + "##" + std::to_string(t) + "-" + std::to_string(k)).c_str())) {
				if (bulk) sim._try_upgrade_bulk(t, k);
				else      sim._try_upgrade(t, k);
			}
		}
        // "Add a Tier N drop".
		Number add_cost = sim._get_cost(t, 3);
		string add_btn;
		if (t == 1) add_btn = (sim.drops[1] <= ZERO)
			                      ? "Add a Tier 1 drop. [FREE - first one]"
			                      : "Add a Tier 1 drop. [Cost: " + N2S(add_cost) + "]";
		else        add_btn = "10 Tier " + std::to_string(t - 1) + " -> 1 Tier " + std::to_string(t) + " [Cost: " + N2S(add_cost) + "]";
		
		bool add_locked = (t != 1 && sim.level < 50);
		if (add_locked) ImGui::Text("(unlocked after SM 8: SL >= 20)");
		else if (AlphaButton((add_btn + "##" + std::to_string(t) + "-").c_str())) {
			if (bulk) sim._try_upgrade_bulk(t, 3);
			else      sim._try_upgrade(t, 3);
		}
		// ---- Synthesize button (added BELOW the buy button) ----
		// 10 Tier t -> 1 Tier (t+1). Unrestricted: no SM8 gate, no SP cost.
		string syn_btn = std::string("10 Tier ") + std::to_string(t) + " -> 1 Tier " + std::to_string(t + 1) + " [Merge x10]";
		bool can_syn = sim_has_ten(sim.drops[t]);
		if (!can_syn) ImGui::BeginDisabled(true);
		if (AlphaButton((syn_btn + "##syn" + std::to_string(t) + "-").c_str())) {
			if (bulk) sim._try_synthesize_bulk(t);
			else      sim._try_synthesize(t);
		}
		if (!can_syn) ImGui::EndDisabled();
		ImGui::EndChild();
	}
	ImGui::EndChild();
	ImGui::Separator();
	// ---- Global upgrades: BOTH cards shown, one per row (two rows). ----
	// Each Global level shows "Base(+bonus)" where bonus = SL(+Ach.47).
	const char* glob_names[2] = { "Global (Am)", "Global (AP)" };
	const char* glob_res[2]   = { "Am", "AP" };
	std::string _gb_str = N2S(sim.global_level_bonus);
	for (int g = 0; g < 2; g++) {
		ImGui::BeginChild(("##glob" + std::to_string(g)).c_str(), ImVec2(-1, 96.0f), true);
		ImGui::Text("%s", glob_names[g]);
		{
			Number* _gr = (g == 0) ? sim.am_ref : sim.ap_ref;
			std::string _hv = _gr ? N2S(*_gr) : std::string("--");
			ImGui::SameLine();
			ImGui::Text("Have: %s %s", _hv.c_str(), glob_res[g]);
		}
		
		int gid = -(g + 1);

		for (int k = 0; k < 2; k++) {
			Number cost_k = sim._get_cost(gid, k);
			ImGui::Text("%s Level: %s(+%s)", (k == 0 ? "Value" : "Gravity"),
			            N2S(sim.level_global[g][k].level).c_str(), _gb_str.c_str());
			string btn = std::string("Buy [Cost: ") + N2S(cost_k) + " " + glob_res[g] + "]";
			if (AlphaButton((btn + "##global" + std::to_string(g) + "-" + std::to_string(k)).c_str())) {
				if (bulk) sim._try_upgrade_bulk(gid, k);
				else      sim._try_upgrade(gid, k);
			}
		}
		ImGui::EndChild();
	}
	ImGui::EndChild();
}

// Alpha tab.
void DrawAlphaTab(player_& player) {
	ImGui::SetNextWindowPos(ImVec2(0, 42));
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - 42));
	ImGui::Begin("##Alpha", nullptr,
	             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
	             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

	ImGui::Text("Matter: %s  ( +%s/s", N2S(player.pre_alpha.score).c_str(), N2S(rate).c_str());
	if(player.unlocked_beta) {
		string positive = "";
		if(player.rate_Number > ZERO) positive = "+";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),"->%s%s/s", positive.c_str(), N2S(player.rate_Number).c_str());
	}
	ImGui::SameLine();
	ImGui::Text(")");

	if(player.unlocked_beta) {
		string s = "x";
		if(player.pre_alpha.score >= TEN.pow(TEN)) s = "^";
		if(player.pre_alpha.score >= TEN.pow(TEN.pow(TEN))) s = "^^";
		if(player.pre_alpha.score >= TEN.pow(TEN.pow(TEN.pow(TEN)))) s = "^^^";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),
		                   "Your Matters are decaying. Matter: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.pre_alpha.score, ONE)).c_str());
		if(player.upgrade[34].level.is_one()) {
			s = "x";
			if(player.alpha.score >= TEN.pow(TEN)) s = "^";
			if(player.alpha.score >= TEN.pow(TEN.pow(TEN))) s = "^^";
			if(player.alpha.score >= TEN.pow(TEN.pow(TEN.pow(TEN)))) s = "^^^";
			ImGui::SameLine();
			ImGui::TextColored(_get_BD_color(),
			                   " Am: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.alpha.score, ONE)).c_str());
		}
		s = "x";
		if(player.beta.score >= TEN.pow(TEN)) s = "^";
		if(player.beta.score >= TEN.pow(TEN.pow(TEN))) s = "^^";
		if(player.beta.score >= TEN.pow(TEN.pow(TEN.pow(TEN)))) s = "^^^";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),
		                   " Bm: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.beta.score, ONE)).c_str());
	}

	ImGui::Text("GIM = 10-logPF(log10(Prest.)) = %s -> ^%s to Mult, PIM: %s -> PF = %s.",
	            N2S(player._get_GIM()).c_str(), N2S(player._get_GIM_effect()).c_str(),
	            N2S(player._get_PIM()).c_str(), N2S(player._get_PF()).c_str());

	ImGui::Separator();

	// For subtabs.
	if(AlphaButton(("Alpha Upgrades"))) player.alpha.tab = 1;
	ImGui::SameLine();
	if(AlphaButton(("Break Alpha Upgrades"))) player.alpha.tab = 2;
	ImGui::SameLine();
	if(player.alpha.unlocked_simulator) {
		if(AlphaButton(("Alpha Simulator"))) player.alpha.tab = 3;
		ImGui::SameLine();
	}
	if(AlphaButton(("Alpha Milestone"))) player.alpha.tab = 4;

	// Alpha Upgrades tab.
	if(player.alpha.tab == 1) {
		ImGui::Text("You have %s AP and %s Alpha.",
		            N2S(player.alpha.point).c_str(),
		            N2S(player.alpha.alpha).c_str());
		ImGui::Text("Respec AUP Upgrades(which also performs an Alpha reset) to redistribute them.");
		ImGui::SameLine();
		if(AlphaButton("Respec")) {
			respec_alpha(player);
			for(int i = 22; i < 26; i ++) player.upgrade[i].level = ZERO;
		}
		for (int i = 0; i < 4; ++i) {
			ImGui::BeginChild(("##au" + to_string(i)).c_str(), ImVec2(0, 70), true);
			ImGui::Text("%s", au_names[i].c_str());
			ImGui::Text("Bought %s.",
			            N2S(player.upgrade[i + 22].level).c_str());
			ImGui::Text("Currently: %s",
			            N2S(player._get_AU_effect(i)).c_str());
			ImGui::SameLine();
			if (AlphaButton(("Buy AU" + to_string(i + 1)).c_str())) player._try_upgrade(i + 22);
			ImGui::SameLine();
			if (AlphaButton(("Buy Max AU" + to_string(i + 1)).c_str())) player._try_upgrade_bulk(i + 22);
			ImGui::EndChild();
		}
		ImGui::Text("You have %s/%s avaliable Alpha Upgrade Points.",
		            N2S(player._get_AUP_avaliable()).c_str(),
		            N2S(player._get_AUP()).c_str());
		for (int i = 0; i < 4; ++i) {
			int id = i + 26;
			ImGui::BeginChild(("##aup" + to_string(i)).c_str(), ImVec2(300, 120), true);
			ImGui::Text("%s", aup_names[i].c_str());
			ImGui::Text("Bought %s",
			            N2S(player.upgrade[id].level).c_str());

			Number cost = player._get_AUP_cost(id);
			ImGui::Text("Cost: %s%s",
			            N2S(cost).c_str(),
			            player._get_AUP_unit(id));

			if (AlphaButton(("Buy AUP" + to_string(i + 1)).c_str()))       player._try_upgrade(id);
			ImGui::SameLine();
			if (AlphaButton(("Buy Max AUP" + to_string(i + 1)).c_str()))   player._try_upgrade_bulk(id);
			ImGui::EndChild();
			if (i != 3) ImGui::SameLine();
		}

		ImGui::Separator();

		// Prestige line.
		if(player.unlocked_prestige) {
			if(player.pre_alpha.highest >= player.pre_alpha.score) {
				string s = " (Resets A1-A8, I1-I8)";
				if(player.upgrade[16].level.is_one()) s = " (Resets A1-A8, I1-I8 and Aeromagnifiers)";
				if(player._get_AM(0))  s = "";
				ImGui::Text("Prestige%s at more than %s matter to gain a further boost.",
				            s.c_str(),
				            N2S(player.pre_alpha.highest).c_str());
			} else {
				string s = " (Resets A1-A8, I1-I8)";
				if(player._get_AM(0))  s = "";
				if(player.upgrade[16].level.is_one()) s = " (Resets A1-A8, I1-I8 and Aeromagnifiers)";
				string s1 = "";
				if(player._get_ach(32)) s1 = ", and is giving Mult. to Amp. by x" + N2S(player._get_PU2_effect(player.pre_alpha.highest));

				ImGui::Text("Prestige%s at %s matter to reduce GIM to %s%s.",
				            s.c_str(),
				            N2S(player.pre_alpha.score).c_str(),
				            N2S(player._get_GIM(player.pre_alpha.score)).c_str(),
				            s1.c_str());
				ImGui::SameLine();
				if(ImGui::Button(("Prestige"))) init_prestige(player);
			}
		} else {
			if(player.upgrade[2].level.is_zero()) ImGui::Text("Obtain at least an A3 to unlock Prestige.");
			else {
				string s = " (Resets A1-A8, I1-I8)";
				if(player.upgrade[16].level.is_one()) s = " (Resets A1-A8, I1-I8 and Aeromagnifiers)";
				if(player._get_AM(0))  s = "";
				string s1 = "";
				if(player._get_ach(32)) s1 = ", and mults Amp. by x" + N2S(player._get_PU2_effect(player.pre_alpha.highest));

				ImGui::Text("Prestige%s at %s matter to reduce GIM to %s%s.",
				            s.c_str(),
				            N2S(player.pre_alpha.score).c_str(),
				            N2S(player._get_GIM(player.pre_alpha.score)).c_str(),
				            s1.c_str());
				ImGui::SameLine();
				if(ImGui::Button(("Prestige"))) init_prestige(player);
			}
		}

		if(player.unlocked_alpha || player.pre_alpha.score >= inf) {
			Number s = player.pre_alpha.score;
			if(player.pre_alpha.score < inf) s = inf;
			ImGui::Text("Reach Alpha at %s matter to gain %s Alpha Points and %s Alpha.",
			            N2S(s).c_str(),
			            N2S(player._get_AP()).c_str(),
			            N2S(player._get_alpha()).c_str());
			ImGui::SameLine();
			if(player.pre_alpha.score >= inf) {
				if(ImGui::Button(("Alpha"))) init_alpha(player);
			}
		}
	}
	// Break Alpha Upgrades tab.
	if(player.alpha.tab == 2) {
		ImGui::Text("You have %s AP and %s Alpha.",
		            N2S(player.alpha.point).c_str(),
		            N2S(player.alpha.alpha).c_str());
		ImGui::BeginChild("##bau", ImVec2(-1, 700), true);
		for (int i = 0; i < 10; ++i) {
			ImGui::BeginChild(("##bau" + to_string(i)).c_str(), ImVec2(-1, 120), true);
			ImGui::Text("%s", bau_names[i].c_str());

			string effect;
			if (i == 0) {
				effect = "Currently: x" + N2S(player._get_BAU_effect(0));
			} else if (i == 1) {
				effect = "Currently: x" + N2S(player._get_BAU_effect(1));
			} else if (i == 2) {
				// BAU3
				Number mul = ONE;
				Number s = player.pre_alpha.score;
				if (s > ONE) {
					Number log_s = s.log(TEN);
					if (log_s > ZERO) {
						Number x = player.upgrade[32].level;
						Number exponent = 0.75 - 0.75 / Number(1.1).pow(x);
						if (exponent > 0.75) exponent = 0.75;
						mul = log_s.pow(exponent);
					}
				}
				effect = "Currently: x" + N2S(mul);
			} else if (i == 3) {
				// BAU4
				Number rate = ZERO;
				if (player.upgrade[33].level > ZERO && player.fastest_ap_per_min > ZERO) {
					rate = player.fastest_ap_per_min * player.upgrade[33].level * 0.01;
				}
				effect = "Currently: " + N2S(rate) + " AP/min";
			} else if (i == 4) {
				// BAU5
				effect = player.upgrade[34].level.is_one()
				         ? "Currently: Unlocked"
				         : "Unlocks Alpha Amplifiers / Alpha Intensifiers.";
			} else if (i == 5) {
				// BAU6
				effect = player.upgrade[35].level.is_one()
				         ? "Currently: Unlocked"
				         : "Unlocks Alpha Aeromagnifiers.";
			} else if (i == 6) {
				// BAU7
				effect = player.upgrade[36].level.is_one()
				         ? "Currently: Enabled"
				         : "Ach. multiplier now applies to Alpha Amplifier.";
			} else if (i == 7) {
				// BAU8
				effect = player.upgrade[37].level.is_one()
				         ? "Currently: x" + N2S(player._get_BAU8_effect())
				         : "Multiplier to Am generation based on AP.";
			} else if (i == 8) {
				// BAU9
				effect = player.upgrade[38].level.is_one()
				         ? "Currently: AIM -" + N2S(player._get_BAU9_effect())
				         : "Reduction to AIM based on AP.";
			} else if (i == 9) {
				// BAU10
				effect = (!player.upgrade[39].level.is_zero())
				         ? "Currently: x" + N2S(player._get_BAU10_effect())
				         : "Multiplier to AP gain based on Am.";
			}
			ImGui::Text("%s", effect.c_str());

			Number cost = player._level_cost(i + 30, player.upgrade[30 + i].level);
			ImGui::Text("Cost: %s AP", N2S(cost).c_str());

			if (player.upgrade[i + 30].level.is_one() && (i >= 4 && i <= 8)) {
				ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Bought");
			} else {
				// Buy One
				if (ImGui::Button(("Buy One##bau" + to_string(i)).c_str()))
					player._try_upgrade(i + 30);
				ImGui::SameLine();
				// Buy Max
				if (ImGui::Button(("Buy Max##bau" + to_string(i)).c_str()))
					player._try_upgrade_bulk(i + 30);
			}

			ImGui::EndChild();
		}
		ImGui::EndChild();
	}
	// Alpha Simulator tab.
	// In replacement to Ach.40-48 rewards.
	if(player.alpha.tab == 3) {
		ImGui::Text("You have %s AP and %s Alpha.",
		            N2S(player.alpha.point).c_str(),
		            N2S(player.alpha.alpha).c_str());
		ImGui::Separator();
		ImGui::BeginChild("##as", ImVec2(-1, -1), true);
		DrawAlphaSimulator(player);
		ImGui::EndChild();
	}
	// Alpha Milestone tab.
	if(player.alpha.tab == 4) {
		ImGui::Text("You have %s AP and %s Alpha.",
		            N2S(player.alpha.point).c_str(),
		            N2S(player.alpha.alpha).c_str());
		ImGui::BeginChild("##am", ImVec2(-1, 700), true);
		ImGui::Text("You have %s Alpha. ",
		            N2S(player.alpha.alpha).c_str());
		if(player.alpha.unlocked_simulator) {
			ImGui::SameLine();
			ImGui::Text("You are at %s Simulation level.",
			            N2S(player.alpha.simulator.level).c_str());
		}
		for(int i = 0; i < 7; i ++) {
			ImGui::BeginChild(("##am" + to_string(i)).c_str(), ImVec2(-1, 80), true);
			if(!player._get_AM(i)) {
				ImGui::Text("Alpha Milestone %s:", to_string(i + 1).c_str());
				ImGui::Text("%s%s",N2S(am[i]).c_str(),am_name[i].c_str());
			} else {
				ImGui::TextColored(ImVec4(0.3f,1,0.3f,1),"Alpha Milestone %s:", to_string(i + 1).c_str());
				ImGui::TextColored(ImVec4(0.3f,1,0.3f,1),"%s%s",N2S(am[i]).c_str(),am_name[i].c_str());
			}
			if(i == 3) {
				string s = (player.break_alpha) ? "Alpha is broken." : "Alpha is not broken.";
				if(player._get_AM(3)) {
					if(AlphaButton(s.c_str())) player.break_alpha = !player.break_alpha;
				}
			}
			ImGui::EndChild();
		}
		if(player.alpha.unlocked_simulator) {
			for(int i = 0; i < 8; i ++) {
				ImGui::BeginChild(("##sm" + to_string(i)).c_str(), ImVec2(-1, 80), true);
				if(!player._get_SM(i)) {
					ImGui::Text("Simulation Milestone %s:", to_string(i + 1).c_str());
					ImGui::Text("%s%s",N2S(sm[i]).c_str(),sm_name[i].c_str());
				} else {
					ImGui::TextColored(ImVec4(0.3f,1,0.3f,1),"Simulation Milestone %s:", to_string(i + 1).c_str());
					ImGui::TextColored(ImVec4(0.3f,1,0.3f,1),"%s%s",N2S(sm[i]).c_str(),sm_name[i].c_str());
				}
				ImGui::EndChild();
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();
}

static const int BD_WEIGHT_COUNT = 4;

static float bd_clamp_weight(const float w[BD_WEIGHT_COUNT], int idx, float new_val) {
	if (new_val < -1.0f) new_val = -1.0f;
	if (new_val >  1.0f) new_val =  1.0f;

	float others = 0.0f;
	for (int k = 0; k < BD_WEIGHT_COUNT; ++k) {
		if (k != idx) others += w[k];
	}

	// new_val + others >= -BE.
	if (new_val + others < -1 * player._get_BE()) {
		new_val = player._get_BE() - others;
	}
	if (new_val < -1.0f) new_val = -1.0f;
	if (new_val >  1.0f) new_val =  1.0f;

	return new_val;
}

void DrawBetaDecayWeights(player_& player) {
	if (player.upgrade[76].level.is_zero()) return;

	ImGui::BeginChild("##bd_weights", ImVec2(-1, 0), true);

	ImGui::Text("Beta Decay Weights");
	ImGui::TextDisabled("Range [-1, 1]; sum of all four is >= %.2f. Less is better.", -1 * player._get_BE());
	ImGui::Separator();

	// Read from local save.
	float w[BD_WEIGHT_COUNT];
	for (int i = 0; i < BD_WEIGHT_COUNT; ++i) {
		w[i] = (float)player.beta.weights[i]._to_long_double();
		if (w[i] < -1.0f) w[i] = -1.0f;
		if (w[i] >  1.0f) w[i] =  1.0f;
	}

	const char* labels[BD_WEIGHT_COUNT] = {
		"Step 1 (x)",
		"Step 2 (^)",
		"Step 3 (^^)",
		"Step 4 (^^^)"
	};

	// Button + Slider
	for (int i = 0; i < BD_WEIGHT_COUNT; ++i) {
		ImGui::PushID(i);

		// - Button
		if (ImGui::Button("-##bdw_minus", ImVec2(24, 0))) {
			float nv = w[i] - 0.01f;
			// Calculation to prevent accidental violation.
			float others = 0.0f;
			for (int k = 0; k < BD_WEIGHT_COUNT; ++k) if (k != i) others += w[k];
			if (others + nv < -1 * player._get_BE()) nv = -1 * player._get_BE() - others;
			if (nv < -1.0f) nv = -1.0f;
			w[i] = nv;
		}

		ImGui::SameLine();

		// Slider
		ImGui::SetNextItemWidth(320.0f);
		float tmp = w[i];
		if (ImGui::SliderFloat(("##bdw_slider" + std::to_string(i)).c_str(),
		                       &tmp, -1.0f, 1.0f, "%.2f")) {
			w[i] = bd_clamp_weight(w, i, tmp);
		}

		ImGui::SameLine();

		// + Button
		if (ImGui::Button("+##bdw_plus", ImVec2(24, 0))) {
			float nv = w[i] + 0.01f;
			if (nv > 1.0f) nv = 1.0f;
			w[i] = nv;
		}

		ImGui::SameLine();
		ImGui::Text("%s", labels[i]);

		// Write data.
		player.beta.weights[i] = (long double)w[i];

		ImGui::PopID();
	}

	ImGui::Separator();

	// Reset
	float sum = 0.0f;
	for (int i = 0; i < BD_WEIGHT_COUNT; ++i) sum += w[i];

	ImGui::Text("Sum: %.2f  (min %.2f)", sum, -1 * player._get_BE());

	ImGui::SameLine();
	if (ImGui::Button("Reset")) {
		for (int i = 0; i < BD_WEIGHT_COUNT; ++i) {
			player.beta.weights[i] = _ZERO;
		}
	}

	ImGui::EndChild();
}

// Beta tab.
void DrawBetaTab(player_& player) {
	ImGui::SetNextWindowPos(ImVec2(0, 42));
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - 42));
	ImGui::Begin("##Beta", nullptr,
	             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
	             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

	ImGui::Text("Matter: %s  ( +%s/s", N2S(player.pre_alpha.score).c_str(), N2S(rate).c_str());
	if(player.unlocked_beta) {
		string positive = "";
		if(player.rate_Number > ZERO) positive = "+";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),"->%s%s/s", positive.c_str(), N2S(player.rate_Number).c_str());
	}
	ImGui::SameLine();
	ImGui::Text(")");

	if(player.unlocked_beta) {
		string s = "x";
		if(player.pre_alpha.score >= TEN.pow(TEN)) s = "^";
		if(player.pre_alpha.score >= TEN.pow(TEN.pow(TEN))) s = "^^";
		if(player.pre_alpha.score >= TEN.pow(TEN.pow(TEN.pow(TEN)))) s = "^^^";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),
		                   "Your Matters are decaying. Matter: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.pre_alpha.score, ONE)).c_str());
		if(player.upgrade[34].level.is_one()) {
			s = "x";
			if(player.alpha.score >= TEN.pow(TEN)) s = "^";
			if(player.alpha.score >= TEN.pow(TEN.pow(TEN))) s = "^^";
			if(player.alpha.score >= TEN.pow(TEN.pow(TEN.pow(TEN)))) s = "^^^";
			ImGui::SameLine();
			ImGui::TextColored(_get_BD_color(),
			                   " Am: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.alpha.score, ONE)).c_str());
		}
		s = "x";
		if(player.beta.score >= TEN.pow(TEN)) s = "^";
		if(player.beta.score >= TEN.pow(TEN.pow(TEN))) s = "^^";
		if(player.beta.score >= TEN.pow(TEN.pow(TEN.pow(TEN)))) s = "^^^";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),
		                   " Bm: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.beta.score, ONE)).c_str());
	}

	ImGui::Text("GIM = 10-logPF(log10(Prest.)) = %s -> ^%s to Mult, PIM: %s -> PF = %s.",
	            N2S(player._get_GIM()).c_str(), N2S(player._get_GIM_effect()).c_str(),
	            N2S(player._get_PIM()).c_str(), N2S(player._get_PF()).c_str());

	ImGui::Separator();

	//For subtabs.
	if(BetaButton(("Beta Decay Information"))) player.beta.tab = 1;
	ImGui::SameLine();
	if(BetaButton(("Beta Upgrades"))) player.beta.tab = 2;
	ImGui::SameLine();
	if(BetaButton(("Beta Challenges"))) player.beta.tab = 3;
	ImGui::SameLine();
	if(BetaButton(("Beta Cosmology"))) player.beta.tab = 4;
	ImGui::SameLine();
	if(BetaButton(("Beta Milestone"))) player.beta.tab = 5;

	ImVec4 dark_red = _get_BD_color();
	// Beta Decay tab.
	if(player.beta.tab == 1) {
		ImGui::Text("You have %s BP and %s Beta.",
		            N2S(player.beta.point).c_str(),
		            N2S(player.beta.beta).c_str());
		ImGui::TextColored(dark_red, "Beta Decay is applied since your 2nd Beta.");
		ImGui::TextColored(dark_red, "Beta Decay effect is affected by four factors: Beta Decay Number(BDN), Beta Decay Factor(BDF), Beta Decay Instability Magnitude(BDIM),\nand your matter amount.");
		ImGui::TextColored(dark_red, "Currently: BDN=%s, BDF=%s, BDIM=%s.\n", N2S(player._get_BDN()).c_str(), N2S(player._get_BDF()).c_str(), N2S(player._get_BDIM()).c_str());

		ImGui::Text("");
		ImGui::TextColored(dark_red, "Define a = Matter you have (Matter, Am, Bm), k = [BDN^((10-x)/9)*BDF^((x-1)/9)]^(BDIM/10).");
		ImGui::TextColored(dark_red, "Beta Decay is divided into four steps.");
		ImGui::TextColored(dark_red, "Step 1: If a >= 10, x = log10(a), a is multiplied by k per second.");
		ImGui::TextColored(dark_red, "Step 2: If a >= 1e10, x = log10(log10(a)), a is empowered by k per second.");
		ImGui::TextColored(dark_red, "Step 3: If a >= e10000000000, x = triple-log10(a), a is dilated by k per second.");
		ImGui::TextColored(dark_red, "Step 4: If a >= e1e10000000000, x = quadrable-log10(a), a is tetrated by k per second.\n");

		ImGui::Text("");
		string s;
		if(player._get_BD_step(player.pre_alpha.score) == ONE) s = "1. Matter/s is being multiplied by ";
		if(player._get_BD_step(player.pre_alpha.score) == 2) s = "2. Matter/s is being empowered by ";
		if(player._get_BD_step(player.pre_alpha.score) == 3) s = "3. Matter/s is being dilated by ";
		if(player._get_BD_step(player.pre_alpha.score) == 4) s = "4. Matter/s is being tetrated by ";
		ImGui::TextColored(dark_red, "You have %s matter, which is in Step %s%s. Current Matter is affected by the same way per second.",
		                   N2S(player.pre_alpha.score).c_str(),
		                   s.c_str(),
		                   N2S(player._get_BD_effect(player.pre_alpha.score, ONE)).c_str());

		if(player._get_BD_step(player.alpha.score) == ONE) s = "1. Alpha Matter/s is being multiplied by ";
		if(player._get_BD_step(player.alpha.score) == 2) s = "2. Alpha Matter/s is being empowered by ";
		if(player._get_BD_step(player.alpha.score) == 3) s = "3. Alpha Matter/s is being dilated by ";
		if(player._get_BD_step(player.alpha.score) == 4) s = "4. Alpha Matter/s is being tetrated by ";
		ImGui::TextColored(dark_red, "You have %s Alpha matter, which is in Step %s%s. Current Alpha Matter is affected by the same way per second.",
		                   N2S(player.alpha.score).c_str(),
		                   s.c_str(),
		                   N2S(player._get_BD_effect(player.alpha.score, ONE)).c_str());

		if(player._get_BD_step(player.beta.score) == ONE) s = "1. Beta Matter/s is being multiplied by ";
		if(player._get_BD_step(player.beta.score) == 2) s = "2. Beta Matter/s is being empowered by ";
		if(player._get_BD_step(player.beta.score) == 3) s = "3. Beta Matter/s is being dilated by ";
		if(player._get_BD_step(player.beta.score) == 4) s = "4. Beta Matter/s is being tetrated by ";
		ImGui::TextColored(dark_red, "You have %s Beta matter, which is in Step %s%s. Current Beta Matter is affected by the same way per second.",
		                   N2S(player.beta.score).c_str(),
		                   s.c_str(),
		                   N2S(player._get_BD_effect(player.beta.score, ONE)).c_str());


		if (!player.upgrade[76].level.is_zero()) {
			ImGui::Separator();
			DrawBetaDecayWeights(player);
		}
	}
	// Beta Upgrades tab.
	if(player.beta.tab == 2) {
		ImGui::Text("You have %s BP and %s Beta.",
		            N2S(player.beta.point).c_str(),
		            N2S(player.beta.beta).c_str());
		ImGui::BeginChild("##bu", ImVec2(-1, 700), true);
		for(int i = 0; i < 1; i ++) {
			ImGui::BeginChild(("##bu" + to_string(i)).c_str(), ImVec2(-1, 120), true);
			ImGui::Text("%s", bu_names[i].c_str());

			string effect;
			if(i == 0) {
				effect = player.upgrade[76].level.is_one()
				         ? "Currently: Unlocked"
				         : "Gains 1 Beta Energy.";
			}

			ImGui::Text("%s", effect.c_str());

			Number cost = player._level_cost(i + 76, player.upgrade[76 + i].level);
			ImGui::Text("Cost: %s BP", N2S(cost).c_str());

			if (player.upgrade[i + 76].level.is_one()) {
				ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Bought");
			} else {
				// Buy One
				if (ImGui::Button(("Buy One##bau" + to_string(i)).c_str()))
					player._try_upgrade(i + 76);
				ImGui::SameLine();
				// Buy Max
				if (ImGui::Button(("Buy Max##bau" + to_string(i)).c_str()))
					player._try_upgrade_bulk(i + 76);
			}
			ImGui::EndChild();
		}
		ImGui::EndChild();
	}
	// Beta Challenges tab.
	if(player.beta.tab == 3) {
	}
	// Beta Cosmology Tree tab.
	if(player.beta.tab == 4) {
	}
	// Beta Milestone tab.
	if(player.beta.tab == 5) {
		ImGui::Text("You have %s BP and %s Beta.",
		            N2S(player.beta.point).c_str(),
		            N2S(player.beta.beta).c_str());
		ImGui::BeginChild("##bm", ImVec2(-1, 700), true);
		ImGui::Text("You have %s Beta.",
		            N2S(player.beta.beta).c_str());
		for(int i = 0; i < 10; i ++) {
			ImGui::BeginChild(("##bm" + to_string(i)).c_str(), ImVec2(-1, 80), true);
			if(player._get_BM(i)) {
				ImGui::Text("Beta Milestone %s:", to_string(i + 1).c_str());
				ImGui::Text("%s%s",N2S(bm[i]).c_str(),bm_name[i].c_str());
			} else {
				ImGui::TextColored(ImVec4(0.3f,1,0.3f,1),"Beta Milestone %s:", to_string(i + 1).c_str());
				ImGui::TextColored(ImVec4(0.3f,1,0.3f,1),"%s%s",N2S(bm[i]).c_str(),bm_name[i].c_str());
			}
			ImGui::EndChild();
		}
		ImGui::EndChild();
	}
	ImGui::End();
}

int storytab = 1;

// Achievement tab.
void DrawStoryTab(player_& player) {
	ImGui::SetNextWindowPos(ImVec2(0, 42));
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - 42));
	ImGui::Begin("##Story", nullptr,
	             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
	             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

	if(ImGui::Button(("Achievements"))) storytab = 1;
	ImGui::SameLine();
	if(ImGui::Button(("Story"))) storytab = 2;

	//The Ach. tab.
	if(storytab == 1) {
		ImGui::BeginChild("##ach", ImVec2(-1, 700), true);
		ImGui::Text("Achievements give Multipliers to:");
		ImGui::Text("Amplifiers: x%s", N2S(player._get_ach_effect(1)).c_str());
		if(player._get_SM(4)) ImGui::Text("Alpha Intensifiers: x%s", N2S(player._get_ach_effect(1)).c_str());
		for(int i = 1; i <= 52; i ++) {
			ImGui::BeginChild(("##ach" + to_string(i)).c_str(), ImVec2(300, 120), true);
			if(player._get_ach(i)) {
				ImGui::TextColored(ImVec4(0.3f,1,0.3f,1),"%s",ach_name[i].c_str());
				ImGui::Separator();
				ImGui::TextColored(ImVec4(0.3f,1,0.3f,1),"%s",ach_desc[i].c_str());
				ImGui::TextColored(ImVec4(0.3f,1,0.3f,1),"%s",ach_reward[i].c_str());
			} else {
				ImGui::Text("%s",ach_name[i].c_str());
				ImGui::Separator();
				ImGui::Text("%s",ach_desc[i].c_str());
				ImGui::Text("%s",ach_reward[i].c_str());
			}
			ImGui::EndChild();
			if(i % 4) ImGui::SameLine();
		}
		ImGui::EndChild();
	}

	//The story tab.
	if(storytab == 2) {
		ImGui::BeginChild("##story", ImVec2(-1, 700), true);
		ImGui::Text("You have unlocked %s stories.", N2S(player._get_story_done()).c_str());
		for(int i = 1; i <= 6; i ++) {
			ImGui::BeginChild(("##story" + to_string(i)).c_str(), ImVec2(-1, 300), true);
			if(!player._get_story(i)) {
				ImGui::TextColored(ImVec4(0.5,0.5,0.5,1),"Story %s", to_string(i).c_str());
				ImGui::Separator();
				ImGui::BeginChild(("##storytext" + to_string(i)).c_str(), ImVec2(-1, 150), true);
				ImGui::TextColored(ImVec4(0.5,0.5,0.5,1),"%s",story_hint[i].c_str());
				ImGui::EndChild();
				ImGui::Separator();
				ImGui::TextColored(ImVec4(0.5,0.5,0.5,1),"%s",story_reward[i].c_str());
			} else {
				ImGui::Text("Story %s", to_string(i).c_str());
				ImGui::Separator();
				ImGui::BeginChild(("##storytext" + to_string(i)).c_str(), ImVec2(-1, 150), true);
				ImGui::Text("%s",story_text[i].c_str());
				ImGui::EndChild();
				ImGui::Separator();
				ImGui::Text("%s",story_reward[i].c_str());
			}
			ImGui::EndChild();
		}
		ImGui::EndChild();
	}

	ImGui::End();
}

void DrawTopBar(player_& player) {
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 42));
	ImGui::Begin("##TopBar", nullptr,
	             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
	             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
	             ImGuiWindowFlags_NoBringToFrontOnFocus);

	if (ImGui::Button("Production", ImVec2(120, 30))) player.currenttab = 1;
	ImGui::SameLine();
	if (ImGui::Button("Options", ImVec2(120, 30))) player.currenttab = 2;
	ImGui::SameLine();
	if (ImGui::Button("Achievements", ImVec2(120, 30))) player.currenttab = 3;
	if (player.unlockedtabs >= 4) {
		ImGui::SameLine();
		if (ImGui::Button("Prestige", ImVec2(120, 30))) player.currenttab = 4;
	}
	if (player.unlockedtabs >= 5) {
		ImGui::SameLine();
		if (AlphaButton("Alpha", ImVec2(120, 30))) player.currenttab = 5;
	}
	if (player.unlockedtabs >= 6) {
		ImGui::SameLine();
		if (BetaButton("Beta", ImVec2(120, 30))) player.currenttab = 6;
	}
	ImGui::End();
}

void GUI_RenderFrame(player_& player) {
	// ---- 1. Message ----
	MSG msg;
	while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
		if (msg.message == WM_QUIT) g_gui_running = false;
	}
	if (!g_gui_running) return;

	// ---- 2. resize ----
	if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
		CleanupRenderTarget();
		g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
		g_ResizeWidth = g_ResizeHeight = 0;
		CreateRenderTarget();
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// ---- 3. Splash / UI
	bool show_splash = false;
	if (!g_splash_done) {
		double elapsed = _get_time() - g_splash_start;
		if (elapsed >= SPLASH_DURATION_MS) {
			g_splash_done = true;
		} else {
			show_splash = true;

			double alpha = 1.0;
			if (elapsed < SPLASH_FADE_MS) {
				alpha = elapsed / SPLASH_FADE_MS;
			} else if (elapsed > SPLASH_DURATION_MS - SPLASH_FADE_MS) {
				alpha = (SPLASH_DURATION_MS - elapsed) / SPLASH_FADE_MS;
			}
			if (alpha < 0.0) alpha = 0.0;
			if (alpha > 1.0) alpha = 1.0;

			ImGuiIO& io = ImGui::GetIO();
			ImVec2 screen = io.DisplaySize;

			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(screen);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 1));
			ImGui::Begin("##splash", nullptr,
			             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
			             ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBringToFrontOnFocus);

			if (g_splash_srv) {
				ImVec2 img_size(1920, 1080);
				float scale = std::min(screen.x / img_size.x, screen.y / img_size.y);
				float w = img_size.x * scale;
				float h = img_size.y * scale;
				ImVec2 pos((screen.x - w) * 0.5f, (screen.y - h) * 0.5f);

				ImDrawList* dl = ImGui::GetWindowDrawList();
				dl->AddImage((ImTextureID)g_splash_srv, pos,
				             ImVec2(pos.x + w, pos.y + h),
				             ImVec2(0, 0), ImVec2(1, 1),
				             IM_COL32(255, 255, 255, (int)(alpha * 255)));
			}
			ImGui::End();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
		}
	}

	if (!show_splash) {
		DrawTopBar(player);

		switch (player.currenttab) {
			case 1:
				DrawMatterTab(player);
				break;
			case 2:
				DrawOptionsTab(player);
				break;
			case 3:
				DrawStoryTab(player);
				break;
			case 4:
				DrawPrestigeTab(player);
				break;
			case 5:
				DrawAlphaTab(player);
				break;
			case 6:
				DrawBetaTab(player);
				break;
		}

		RenderNotifications();
	}

	ImGui::Render();
	float clear_color[4];
	if (show_splash) {
		clear_color[0]=0;
		clear_color[1]=0;
		clear_color[2]=0;
		clear_color[3]=1;
	} else             {
		clear_color[0]=0.1f;
		clear_color[1]=0.1f;
		clear_color[2]=0.12f;
		clear_color[3]=1.0f;
	}
	g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
	g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	g_pSwapChain->Present(1, 0);
}
#endif // _WIN32

// ============================================================
// Section 7 - Game-loop function.
// ============================================================

vector <int> a;
static bool prevP = false;
static bool prevA = false;
static bool prevL = false;
static bool prevR = false;
static bool prevUp = false;
static bool prevDown = false;
static bool prevEnter = false;
static bool prevLeftEdit = false;
static bool prevRightEdit = false;
static bool prevShiftEnter = false;
static bool prevKeyState[256] = {false};
int cur_option = 1;

static int debug_cnt = 0;

bool imgui_capturing = false;

//Internal tick game update.
void update(player_& player, Number s) {
#ifdef _WIN32
	imgui_capturing = g_use_gui
	                  && ImGui::GetCurrentContext()
	                  && ImGui::GetIO().WantCaptureKeyboard;
#else
	imgui_capturing = false;
#endif
	//Constants update.
	player._update_PF();
	player._update_PIM();
	player._update_GIM();
	player._update_AIM();
	//Music state update.
	int song_idx = (int)player.options[4].val._to_long_double();
	int song_vol = (int)player.options[5].val._to_long_double();
	if (song_idx != current_bgm_index) {
		current_bgm_index = song_idx;
		play_bgm(song_idx, song_vol);
		current_bgm_volume = song_vol;
	} else if (song_vol != current_bgm_volume) {
		current_bgm_volume = song_vol;
		set_bgm_volume(song_vol);
	}

	//Whether a key is being pressed.
	auto isKeyDown = [](int vKey)->bool {
		if (imgui_capturing) return false;
		HWND hForeground = GetForegroundWindow();
		if (!hForeground) return false;
		DWORD fgPid = 0;
		GetWindowThreadProcessId(hForeground, &fgPid);
		return ((GetAsyncKeyState(vKey) & 0x8000) != 0)
		&& (fgPid == GetCurrentProcessId());
	};
	bool upNow    = isKeyDown(VK_UP);
	bool downNow  = isKeyDown(VK_DOWN);
	bool enterNow = isKeyDown(VK_RETURN);
	bool leftNow  = isKeyDown(VK_LEFT);
	bool rightNow = isKeyDown(VK_RIGHT);
	bool shiftNow = isKeyDown(VK_SHIFT);
	//Options tab render.
	if (player.currenttab == 2) {
		int sel = (int)player.options[0].val._to_long_double();
		if (upNow && !prevUp) {
			sel = (sel + 4) % 5;
			player.options[0].val = sel;
			editing_option = false;
		}
		if (downNow && !prevDown) {
			sel = (sel + 1) % 5;
			player.options[0].val = sel;
			editing_option = false;
		}
		if (enterNow && !prevEnter && !shiftNow) {
			editing_option = !editing_option;
		}
		if (editing_option) {
			if (sel == 0) { // Time flux
				long double v = player.options[1].val._to_long_double();
				if (leftNow  && !prevLeftEdit)  v -= 1.0L;
				if (rightNow && !prevRightEdit) v += 1.0L;
				if (v < 0) v = 0;
				player.options[1].val = v;
			} else if (sel == 1) {
				long double v = player.options[2].val._to_long_double();
				if (leftNow  && !prevLeftEdit)  v -= 1.0L;
				if (rightNow && !prevRightEdit) v += 1.0L;
				if (v < 0)   v = 0;
				if (v > 100) v = 100;
				player.options[2].val = v;
			} else if (sel == 2) { // Save file name
				for (int vk = 'A'; vk <= 'Z'; ++vk) {
					bool kd = isKeyDown(vk);
					if (kd && !prevKeyState[vk]) {
						char c = shiftNow ? (char)vk : (char)(vk - 'A' + 'a');
						player.options[3].val_s += c;
					}
					prevKeyState[vk] = kd;
				}
				for (int vk = '0'; vk <= '9'; ++vk) {
					bool kd = isKeyDown(vk);
					if (kd && !prevKeyState[vk]) {
						player.options[3].val_s += (char)vk;
					}
					prevKeyState[vk] = kd;
				}
				bool sp = isKeyDown(VK_SPACE);
				if (sp && !prevKeyState[VK_SPACE]) player.options[3].val_s += ' ';
				prevKeyState[VK_SPACE] = sp;
				bool bk = isKeyDown(VK_BACK);
				if (bk && !prevKeyState[VK_BACK]) {
					if (!player.options[3].val_s.empty())
						player.options[3].val_s.pop_back();
				}
				prevKeyState[VK_BACK] = bk;
				bool dot = isKeyDown(VK_OEM_PERIOD);
				if (dot && !prevKeyState[VK_OEM_PERIOD]) player.options[3].val_s += '.';
				prevKeyState[VK_OEM_PERIOD] = dot;
				bool underscore = isKeyDown(VK_OEM_MINUS) && shiftNow;
				if (underscore && !prevKeyState[VK_OEM_MINUS]) player.options[3].val_s += '_';
				prevKeyState[VK_OEM_MINUS] = underscore;
			} else if(sel == 3) { // BGM Select
				long double v = player.options[4].val._to_long_double();
				if (leftNow  && !prevLeftEdit)  v -= 1.0L;
				if (rightNow && !prevRightEdit) v += 1.0L;
				if (v < 0)   v = 0;
				if (v > (long double)(BGM_LIST.size() - 1)) v = (long double)(BGM_LIST.size() - 1);
				player.options[4].val = v;
			} else if(sel == 4) { //BGM Volume
				long double v = player.options[5].val._to_long_double();
				if (leftNow  && !prevLeftEdit)  v -= 1.0L;
				if (rightNow && !prevRightEdit) v += 1.0L;
				if (v < 0)   v = 0;
				if (v > 100) v = 100;
				player.options[5].val = v;
			}
		}
		bool shiftEnter = shiftNow && enterNow;
		if (shiftEnter && !prevShiftEnter) {
			long double pool_amount = 0;
			if (sel == 0) {
				pool_amount = player.options[1].val._to_long_double();
			} else if (sel == 1) {
				pool_amount = player.time_flux * player.options[2].val._to_long_double() / 100.0L;
			}
			if (pool_amount > player.time_flux) pool_amount = player.time_flux;
			if (pool_amount > 0) {
				player.time_flux -= pool_amount;
				flux_pool = pool_amount;
				flux_elapsed = 0;
			}
		}
		prevShiftEnter = shiftEnter;
	}
	if (!(player.currenttab == 2 && editing_option)) {
		if (rightNow && !prevR) {
			player.currenttab = (player.currenttab) % (player.unlockedtabs) + 1;
			editing_option = false;
		}
		if (leftNow && !prevL) {
			player.currenttab = (player.currenttab + player.unlockedtabs - 2) % (player.unlockedtabs) + 1;
			editing_option = false;
		}
	}
	bool block_hotkeys = (player.currenttab == 2 && editing_option);

	bool capsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;

	//Prestige, Alpha, etc.
	bool pNow = isKeyDown('P');
	bool aNow = isKeyDown('A');
	if (pNow && !prevP && capsOn) {
		if (player.unlocked_prestige && player.pre_alpha.score > player.pre_alpha.highest) {
			player.pre_alpha.highest = max(player.pre_alpha.score, player.pre_alpha.highest);
			init_prestige(player);
		}
	}
	if (aNow && !prevA && capsOn) {
		if (player.pre_alpha.score >= inf) {
			init_alpha(player);
		}
	}
	//A1-A8, I1-I8 upgrade.
	for (int i = 0; i < 8; ++i) {
		// Manual hot key
		if (!block_hotkeys && isKeyDown('A') && isKeyDown('1' + i)) {
			if (g_bulk_buy) {
				player._try_upgrade(i);
				Number cur = player.pre_alpha.score;
				player._try_upgrade_bulk(i);
				if(cur >= 1e100) player.pre_alpha.score = cur;
			}
			Number x = player.upgrade[i].level;
			player._try_upgrade(i);
			while(x != player.upgrade[i].level) {
				x = player.upgrade[i].level;
				player._try_upgrade(i);
			}
		}
		if (!block_hotkeys && isKeyDown('I') && isKeyDown('1' + i)) {
			if (g_bulk_buy) {
				player._try_upgrade(i + 8);
				Number cur = player.pre_alpha.score;
				player._try_upgrade_bulk(i + 8);
				if(cur >= 1e100) player.pre_alpha.score = cur;
			}
			Number x = player.upgrade[i + 8].level;
			player._try_upgrade(i + 8);
			while(x != player.upgrade[i + 8].level) {
				x = player.upgrade[i + 8].level;
				player._try_upgrade(i + 8);
			}
		}
		// Autobuyer
		if (player.upgrade[i].autobuyer.enabled && player._get_AM(1)) {
			Number cur = player.pre_alpha.score;
			player._try_upgrade_bulk(i);
			if(cur >= 1e100) player.pre_alpha.score = cur;
			Number x = player.upgrade[i].level;
			player._try_upgrade(i);
			while(x != player.upgrade[i].level) {
				x = player.upgrade[i].level;
				player._try_upgrade(i);
			}
		}
		if (player.upgrade[i + 8].autobuyer.enabled && player._get_AM(2)) {
			Number cur = player.pre_alpha.score;
			player._try_upgrade_bulk(i + 8);
			if(cur >= 1e100) player.pre_alpha.score = cur;
			Number x = player.upgrade[i + 8].level;
			player._try_upgrade(i + 8);
			while(x != player.upgrade[i + 8].level) {
				x = player.upgrade[i + 8].level;
				player._try_upgrade(i + 8);
			}
		}
	}

	//PUs, Aeros.
	for (int i = 0; i < 4; i ++) if (!block_hotkeys && isKeyDown('P') && isKeyDown('1' + i)) player._try_upgrade(i + 16);
	if (!block_hotkeys && isKeyDown('M')) player._try_upgrade_bulk_all();
	if (((!block_hotkeys && shiftNow && isKeyDown('A'))
	        || (player.upgrade[20].autobuyer.enabled && player._get_AM(5)))
	        && player.upgrade[16].level.is_one()) {
		bool ok = (player._level_cost(20, player.upgrade[20].level) <= player.pre_alpha.score);
		Number cur = player.pre_alpha.score;
		player._try_upgrade_bulk(20);
		if(cur >= 1e100) player.pre_alpha.score = cur;
		Number x = player.upgrade[20].level;
		player._try_upgrade(20);
		while(x != player.upgrade[20].level) {
			x = player.upgrade[20].level;
			player._try_upgrade(20);
		}
		if(ok && !player._get_AM(0)) init_aero(player);
	}
	if (((!block_hotkeys && shiftNow && isKeyDown('I'))
	        || (player.upgrade[21].autobuyer.enabled && player._get_AM(5)))
	        && player.upgrade[16].level.is_one()) {
		bool ok = (player._level_cost(21, player.upgrade[21].level) <= player.pre_alpha.score);
		Number cur = player.pre_alpha.score;
		player._try_upgrade_bulk(21);
		if(cur >= 1e100) player.pre_alpha.score = cur;
		Number x = player.upgrade[21].level;
		player._try_upgrade(21);
		while(x != player.upgrade[21].level) {
			x = player.upgrade[21].level;
			player._try_upgrade(21);
		}
		if(ok && !player._get_AM(0)) init_aero(player);
	}

	//Time and income calculation.
	Number tem = _time_flux_accel(s);
	Number effective_s = player._get_game_speed() * s;

	// Safety clamp for effective_s
	if (effective_s.is_negative() || !std::isfinite(effective_s._to_long_double())) {
		effective_s = ZERO;
	}
	// Alpha Simulator physics: advance every frame while it is unlocked.
	// This call is what populates the balls (first init) and the live drop
	// sprites on the board, and banks the SP earned from collisions. Without
	// it, ball[i].value stays 0 and no drop sprite is ever spawned, so the
	// board renders as an empty frame.
	if (player.alpha.unlocked_simulator) player.alpha.simulator.update(effective_s);
	player.pre_alpha.time += effective_s;
	player.alpha.time += effective_s;
	player.beta.time += effective_s;

	// ** BAU4: Passive AP generation.
	if (player.upgrade[33].level > ZERO && player.fastest_ap_per_min > ZERO) {
		Number ap_per_min = player.fastest_ap_per_min * player.upgrade[33].level * 0.01;
		Number ap_per_sec = ap_per_min / 60.0;
		player.alpha.point += ap_per_sec * effective_s;
		player.alpha.point = player.alpha.point.normalize();
	}

	// ============================================================
	// Amplifier / Intensifier Production & Matter Accumulation.
	// ============================================================
	Number add = ONE;
	for (int i = 0; i < 8; i ++) {

		// Base effect calculation
		Number a = 3
		           * player.upgrade[i].level.normalize()
		           * player._get_aero_effect_a()
		           * player._get_ach_effect(1)
		           * player._get_BAU_effect(1)
		           * player._get_Am_effect(3);

		if (player.upgrade[17].level.is_one()) a = a * player._get_PU2_effect();
		if (player._get_ach(32)) a = a * player._get_PU2_effect(player.pre_alpha.highest);
		if (player._get_story(1)) a = a * player._get_story_effect(1);

		// Intensifier exponent
		Number lev = (player.upgrade[i + 8].level + ONE);
		Number exp_val = player._get_GIM_effect() - ONE; // Usually -0.5 initially

		// Calculate exponent multiplier from intensifiers/aeros
		Number exp_mult = lev.pow_sum(exp_val)
		                  * player._get_aero_effect_i()
		                  * player._get_Am_effect(4);

		Number prod_ai = realify( a.pow(exp_mult) );
		player.pre_alpha.amount[i] = player.pre_alpha.amount[i] + prod_ai * effective_s;

		add = add * player.pre_alpha.amount[i];
	}

	rate = add.pow(player._get_GIM_effect());
	rate = rate.normalize();

	Number score_delta = rate * effective_s;
	score_delta = player._get_BD(score_delta, ONE);

	Number score = player.pre_alpha.score;
	player.pre_alpha.score = player.pre_alpha.score + score_delta;
	player.pre_alpha.score = player.pre_alpha.score.normalize();

	player.pre_alpha.score = player._get_BD(player.pre_alpha.score, effective_s);
	player.rate_Number = (player.pre_alpha.score - score) / effective_s;

	// CRITICAL FIX: Prevent score from dropping below zero or becoming NaN
	if (player.pre_alpha.score.is_negative()) {
		player.pre_alpha.score = _ONE; // Reset to 1 to allow buying again
	}
	if (!player.upgrade[2].level.is_zero()) player.unlocked_prestige = true;

	// ============================================================
	// Alpha Amplifier / Alpha Intensifier Production & Am Accumulation.
	// ============================================================
	if (player.upgrade[34].level.is_one()) {
		Number am_add = ONE;
		for (int i = 0; i < 8; i++) {
			// AA base: 3 * AA level * Alpha Amp. Aero effect.
			Number aa = 3
			            * player.upgrade[40 + i].level
			            * player._get_alpha_aero_effect_a()
			            * player._get_story_effect(3);

			if (player.upgrade[36].level.is_one()) {
				aa = aa * player._get_ach_effect(1);
			}
			if (player._get_SM(1)) {
				aa = aa * player._get_story_effect(1);
			}
			if (player._get_SM(7)) {
				Number base = player.pre_alpha.score;
				Number mult = ONE;
				if(base >= TEN) {
					Number base_log = base.log(TEN);
					if(base_log >= TEN) {
						mult = base_log.log(TEN);
					}
				}
				aa = aa * mult;
			}

			log_line("aa=" + N2S(aa) + ",");

			// AI exponent: uses AIM_effect and Alpha Int. Aero effect.
			Number lev_ai = player.upgrade[48 + i].level + ONE;
			Number exp_ai = lev_ai.pow_sum(player._get_AIM_effect() - 1)
			                * player._get_alpha_aero_effect_i();

			Number effect_ai = aa.pow(exp_ai);

			Number am_delta = effective_s * effect_ai;

			player.alpha.amount[i] = player.alpha.amount[i] + am_delta;
			player.alpha.amount[i] = player.alpha.amount[i].normalize();

			am_add = am_add * player.alpha.amount[i];

			if (std::isnan(am_add._to_long_double())) am_add = ONE;
		}

		Number am_exp = player._get_AIM_effect();
		player.alpha.am_rate = am_add.pow(am_exp).normalize();


		if (player._get_story(4)) {
			player.alpha.am_rate = player.alpha.am_rate * player._get_story_effect(4);
		}
		player.alpha.am_rate = player.alpha.am_rate * player._get_BAU8_effect();
		if (player._get_SM(2)) {
			Number base = player.alpha.am_best;
			if (base > ONE) {
				Number log10_am = base.log(TEN);
				if (log10_am > ONE) {
					player.alpha.am_rate = player.alpha.am_rate * log10_am;
				}
			}
		}
		if (player._get_SM(3)) {
			Number base = player.alpha.point;
			if (base > ONE) {
				Number log10_ap = base.log(TEN);
				if (log10_ap > ONE) {
					player.alpha.am_rate = player.alpha.am_rate * log10_ap;
				}
			}
		}
		Number am_score_delta = player.alpha.am_rate * effective_s;
		am_score_delta = player._get_BD(am_score_delta, ONE);

		Number alpha_score = player.alpha.score;
		player.alpha.score = player.alpha.score + am_score_delta;
		player.alpha.score = player.alpha.score.normalize();

		player.alpha.score = player._get_BD(player.alpha.score, effective_s);

		player.rate_Number_Am = (player.alpha.score - alpha_score) / effective_s;

		if (player.alpha.score.is_negative()) {
			player.alpha.score = ONE;
		}

		if(player.beta.highest_am < player.alpha.score) player.beta.highest_am = player.alpha.score;
	} else {
		// BAU5 not bought: no Am production, no Am effect.
		player.alpha.am_rate = ZERO;
		for (int i = 0; i < 8; i++) player.alpha.amount[i] = ONE;
	}



	// ============================================================
	// Beta Amplifier / Beta Intensifier Production & Bm Accumulation.
	// ============================================================
	if (player.unlocked_beta) {
		Number bm_add = ONE;
		for (int i = 0; i < 8; i ++) {
			Number ba = 3
			            * player.upgrade[58 + i].level
			            * player._get_beta_aero_effect_a()
			            * player._get_story_effect(3);

			Number lev_bi = player.upgrade[66 + i].level + ONE;
			Number exp_bi = lev_bi.pow_sum(player._get_BIM_effect() - 1)
			                * player._get_beta_aero_effect_i();

			Number effect_bi = ba.pow(exp_bi);

			Number bm_delta = effective_s * effect_bi;

			player.beta.amount[i] = player.beta.amount[i] + bm_delta;
			player.beta.amount[i] = player.beta.amount[i].normalize();


			if (player.beta.amount[i].is_negative()) {
				player.beta.amount[i] = ONE;
			}

			bm_add = bm_add * player.alpha.amount[i];

			if (std::isnan(bm_add._to_long_double())) bm_add = ONE;
		}

		Number bm_exp = player._get_BIM_effect();
		player.beta.bm_rate = bm_add.pow(bm_exp).normalize();

		Number bm_score_delta = player.beta.bm_rate * effective_s;
		bm_score_delta = player._get_BD(bm_score_delta, ONE);

		Number beta_score = player.beta.score;
		player.beta.score = player.beta.score + bm_score_delta;
		player.beta.score = player.beta.score.normalize();

		player.beta.score = player._get_BD(player.beta.score, effective_s);

		player.rate_Number_Bm = (player.beta.score - beta_score) / effective_s;

		if (player.beta.score.is_negative()) {
			player.beta.score = ONE;
		}

		if(player.beta.highest_bm < player.beta.score) player.beta.highest_bm = player.beta.score;
	} else {
		player.beta.bm_rate = ZERO;
		for (int i = 0; i < 8; i++) player.beta.amount[i] = ONE;
	}


	// Highests update.
	if (player.alpha.highest < player.pre_alpha.score) player.alpha.highest = player.pre_alpha.score;
	if (player.beta.highest < player.pre_alpha.score) player.beta.highest = player.pre_alpha.score;
	if (player.alpha.score > player.alpha.am_best) {
		player.alpha.am_best = player.alpha.score;
	}

	if (player._get_AM(6) && player.alpha.autobuyer && player.pre_alpha.score >= inf) {
		init_alpha(player);
	}

	player.pre_alpha.score = realify(player.pre_alpha.score);

	// Tries to unlock Achs and Stories.
	player._try_unlock_all();

	if(player._get_ach(40)) player.alpha.unlocked_simulator = true;

	prevP = pNow;
	prevA = aNow;
	prevL = leftNow;
	prevR = rightNow;
	prevUp = upNow;
	prevDown = downNow;
	prevEnter = enterNow;
	prevLeftEdit = leftNow;
	prevRightEdit = rightNow;

}

// ============================================================
// Section 8 - Save-Read Management System.
// ============================================================

static std::string get_timestamp() {
	std::time_t t = std::time(nullptr);
	std::tm* tm_ptr = std::localtime(&t);
	std::ostringstream oss;
	if (tm_ptr) oss << std::put_time(tm_ptr, "%Y-%m-%d-%H%M%S");
	return oss.str();
}

static std::string get_save_string() {
	const std::string tmp_path = "##export_tmp.txt";

	std::string saved_name = player.options[3].val_s;
	player.options[3].val_s = tmp_path;
	save_game_state(player);
	player.options[3].val_s = saved_name;
	if (player.options[3].val_s.empty()) player.options[3].val_s = "save1.txt";

	std::ifstream f(tmp_path, std::ios::binary);
	if (!f) return "";
	std::stringstream ss;
	ss << f.rdbuf();
	f.close();
	std::remove(tmp_path.c_str());
	return ss.str();
}

static void load_game_state_from_string(const std::string& data) {
	const std::string tmp_path = "##import_tmp.txt";
	{
		std::ofstream f(tmp_path, std::ios::binary | std::ios::trunc);
		if (!f) return;
		f << data;
		f.close();
	}
	load_game_state(tmp_path, player);
	std::remove(tmp_path.c_str());
}

#ifdef _WIN32
static void copy_to_clipboard(const std::string& text) {
	if (!OpenClipboard(nullptr)) return;
	EmptyClipboard();
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
	if (hMem) {
		void* p = GlobalLock(hMem);
		if (p) {
			std::memcpy(p, text.c_str(), text.size() + 1);
			GlobalUnlock(hMem);
			SetClipboardData(CF_TEXT, hMem);
		}
	}
	CloseClipboard();
}

static std::string paste_from_clipboard() {
	std::string result;
	if (!OpenClipboard(nullptr)) return result;
	HANDLE hData = GetClipboardData(CF_TEXT);
	if (hData) {
		const char* pszText = static_cast<const char*>(GlobalLock(hData));
		if (pszText) {
			result = pszText;
			GlobalUnlock(hData);
		}
	}
	CloseClipboard();
	return result;
}
#endif

//This part is for Read-Save system.
static long double parse_number(const std::string& s, size_t& pos) {
	size_t start = pos;
	if (pos < s.size() && (s[pos] == '-' || s[pos] == '+')) ++pos;
	while (pos < s.size() &&
	        (std::isdigit((unsigned char)s[pos]) ||
	         s[pos] == '.' || s[pos] == 'e' || s[pos] == 'E')) {
		if (s[pos] == 'e' || s[pos] == 'E') {
			++pos;
			if (pos < s.size() && (s[pos] == '-' || s[pos] == '+')) ++pos;
			while (pos < s.size() && std::isdigit((unsigned char)s[pos])) ++pos;
			break;
		}
		++pos;
	}
	if (pos == start) return 0.0L;
	return std::stold(s.substr(start, pos - start));
}

static Real parse_Real(const std::string& s) {
	std::string str;
	str.reserve(s.size());
	for (char c : s) if (!std::isspace((unsigned char)c)) str += c;
	if (str.empty()) return Real();

	bool sign = true;
	if (str.size() >= 2 && str[0] == '1' && str[1] == '/') {
		sign = false;
		str = str.substr(2);
	}

	if (str.size() >= 3 && str.compare(0, 2, "N(") == 0 && str.back() == ')') {
		std::string inner = str.substr(2, str.size() - 3);

		std::vector<std::string> parts;
		int depth = 0;
		size_t start = 0;
		for (size_t k = 0; k < inner.size(); ++k) {
			char c = inner[k];
			if (c == '(') ++depth;
			else if (c == ')') --depth;
			else if (c == ',' && depth == 0) {
				parts.push_back(inner.substr(start, k - start));
				start = k + 1;
			}
		}
		parts.push_back(inner.substr(start));
		if (parts.size() != 4) return Real();

		try {
			long double v  = std::stold(parts[0]);
			uint64_t    l  = (uint64_t)std::stoull(parts[1]);
			bool        sg = (parts[2] == "1");
			int         n  = (parts[3] == "1") ? 1 : -1;
			if (!sign) sg = !sg;
			return Real(sg, n, v, (long double)l).normalize();
		} catch (...) {
			return Real();
		}
	}

	try {
		long double val = std::stold(str);
		if (val < 0.0L) {
			sign = !sign;
			val  = -val;
		}
		return Real(sign, val, 0.0L).normalize();
	} catch (...) {
		return Real();
	}
}

static Real read_Real(const std::string& s) {
	std::string str;
	str.reserve(s.size());
	for (char c : s) if (!std::isspace((unsigned char)c)) str += c;
	return parse_Real(str);
}

static Number read_Number(const std::string& s) {
	if (s.empty() || s == "0") return Number();

	if (s.back() == 'i') {
		std::string t = s.substr(0, s.size() - 1);

		int split = -1;
		for (int k = (int)t.size() - 1; k > 0; --k) {
			if (t[k] == '+' || t[k] == '-') {
				split = k;
				break;
			}
		}

		std::string real_str, imag_str;
		if (split == -1) {
			real_str.clear();
			imag_str = t;
		} else {
			real_str = t.substr(0, split);
			imag_str = t.substr(split);
		}

		Real re = real_str.empty() ? ZERO : parse_Real(real_str);

		Real im;
		if (imag_str.empty() || imag_str == "+") im = ONE;
		else if (imag_str == "-")                 im = ZERO - ONE;
		else                                      im = parse_Real(imag_str);

		return Number(re, im);
	}

	return Number(parse_Real(s), ZERO);
}


static Number parse_Number(const std::string& s) {
	std::string str;
	str.reserve(s.size());
	for (char c : s) if (!std::isspace((unsigned char)c)) str += c;
	if (str.empty()) return Number();

	if (str.size() >= 4 && str.compare(0, 2, "C(") == 0 && str.back() == ')') {
		std::string inner = str.substr(2, str.size() - 3);

		int depth = 0;
		size_t comma = std::string::npos;
		for (size_t k = 0; k < inner.size(); ++k) {
			char c = inner[k];
			if (c == '(') ++depth;
			else if (c == ')') --depth;
			else if (c == ',' && depth == 0) {
				comma = k;
				break;
			}
		}
		if (comma == std::string::npos) return Number();

		Real re = parse_Real(inner.substr(0, comma));
		Real im = parse_Real(inner.substr(comma + 1));
		return Number(re, im);
	}

	if (str.size() >= 3 && str.compare(0, 2, "N(") == 0 && str.back() == ')') {
		return Number(parse_Real(str), ZERO);
	}

	return read_Number(str);
}

// Windows: MoveFileExA + MOVEFILE_REPLACE_EXISTING + MOVEFILE_WRITE_THROUGH
// POSIX  : std::rename
static bool atomic_replace_file(const std::string& src, const std::string& dst) {
#ifdef _WIN32
	return MoveFileExA(
	           src.c_str(),
	           dst.c_str(),
	           MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
	       ) != 0;
#else
	return std::rename(src.c_str(), dst.c_str()) == 0;
#endif
}

static bool flush_and_close(FILE* f) {
	if (!f) return false;
	if (fflush(f) != 0) {
		fclose(f);
		return false;
	}
#ifdef _WIN32
	_commit(_fileno(f));
#else
	fsync(fileno(f));
#endif
	return fclose(f) == 0;
}


static bool copy_file_contents(const string& src, const string& dst) {
	ifstream in(src, ios::binary);
	if (!in) return false;
	ofstream out(dst, ios::binary | ios::trunc);
	if (!out) return false;
	out << in.rdbuf();
	return out.good();
}

// ----------------------------------------------------------------------
// Fixed Save/Load System for Options and Game State
// ----------------------------------------------------------------------

void save_options(player_& player) {
	const string opt_target = "options.txt";
	const string opt_tmp    = opt_target + ".tmp";

	// Open temporary file for writing
	FILE* opt = fopen(opt_tmp.c_str(), "w");
	if (!opt) {
		cerr << "[ERROR] Could not open " << opt_tmp << " for writing." << endl;
		return;
	}

	for (int i = 0; i < 6; i++) {
		// Format: opt_id <index>
		fprintf(opt, "opt_id %d\n", i);

		if (i != 3) {
			// Save Number value using standard to_save_string
			// Ensure we normalize before saving to avoid weird states
			string val_str = N2S(player.options[i].val);
			fprintf(opt, "%s\n", val_str.c_str());
		} else {
			// Save String value directly (Savefile name)
			// Ensure it's not empty
			string s = player.options[i].val_s;
			if (s.empty()) s = "save1.txt";
			fprintf(opt, "%s\n", s.c_str());
		}
	}

	// Flush and close
	if (flush_and_close(opt)) {
		// Atomically replace the old file
		if (!atomic_replace_file(opt_tmp, opt_target)) {
			cerr << "[WARNING] atomic_replace failed for options.txt, trying copy." << endl;
			copy_file_contents(opt_tmp, opt_target);
			remove(opt_tmp.c_str());
		}
	} else {
		cerr << "[ERROR] Failed to flush/close options temp file." << endl;
		remove(opt_tmp.c_str());
	}
}

void load_options(player_& player) {
	const char* opt_paths[] = { "options.txt", "options.txt.tmp" };
	bool loaded = false;

	// DEBUG: Force reset if you want to ensure clean state.
	// Uncomment the next line if you want to DELETE existing options.txt on every run to test defaults.
	// remove("options.txt");

	for (const char* path : opt_paths) {
		FILE* opt = fopen(path, "r");
		if (!opt) continue;

		cout << "[INFO] Loading options from: " << path << endl;

		char buffer[1024];
		auto read_line = [&]() -> std::string {
			if (fgets(buffer, sizeof(buffer), opt) == nullptr)
				return "";
			std::string line = buffer;
			// Trim trailing whitespace/newlines
			while (!line.empty() && (line.back() == '\n' || line.back() == '\r' || line.back() == ' '))
				line.pop_back();
			return line;
		};

		int count = 0;
		string line = read_line();
		while (line != "") {
			// Check if line starts with "opt_id "
			if (line.substr(0, 7) == "opt_id ") {
				try {
					int id = atoi(line.substr(7).c_str());
					string val_ = read_line(); // Read the next line which is the value

					if (id >= 0 && id < 6) {
						if (id == 3) {
							// String option
							if (!val_.empty()) {
								player.options[id].val_s = val_;
							} else {
								player.options[id].val_s = "save1.txt";
							}
						} else {
							// Number option
							if (!val_.empty()) {
								Number parsed = parse_Number(val_);
								// Sanity check: if parsed is NaN or Inf, ignore it
								if (std::isfinite(parsed._to_long_double())) {
									player.options[id].val = parsed;
								} else {
									cerr << "[WARN] Invalid number in options[" << id << "]: " << val_ << endl;
								}
							}
						}
						count++;
					}
				} catch (...) {
					cerr << "[ERROR] Exception parsing option line: " << line << endl;
				}
			}
			line = read_line();
		}
		fclose(opt);

		// If we loaded at least one option, consider it successful
		if (count > 0) {
			loaded = true;
			cout << "[INFO] Successfully loaded " << count << " options." << endl;

			// Promote .tmp to .txt if we read from tmp
			if (std::string(path) == "options.txt.tmp") {
				atomic_replace_file("options.txt.tmp", "options.txt");
			}
			break;
		} else {
			cout << "[WARN] File " << path << " existed but contained no valid options." << endl;
		}
	}

	// If no options file was found or it was empty/corrupt, set defaults
	if (!loaded) {
		cout << "[INFO] No valid options found. Setting defaults." << endl;

		player.options[0].val = ZERO;          // Mode: Seconds
		player.options[1].val = 60;            // Amount: 60s
		player.options[2].val = 10;            // Amount: 10%
		player.options[3].val_s = "save1.txt"; // Savefile name
		player.options[4].val = 0;             // BGM Index
		player.options[5].val = 50;            // BGM Volume

		// Immediately save these defaults so they persist
		save_options(player);
	}
}

void save_game_state(player_& player) {
	if (player.options[3].val_s.empty()) {
		player.options[3].val_s = "save1.txt";
	}

	const string target = player.options[3].val_s;
	const string tmp    = target + ".tmp";

	FILE* save1 = fopen(tmp.c_str(), "w");
	if (!save1) return;

	// Save Upgrades
	for (int i = 0; i < 80; ++i) {
		fprintf(save1, "upg_level %d\n", i);
		fprintf(save1, "%s\n", N2S_save(player.upgrade[i].level).c_str());

		fprintf(save1, "upg_cost %d\n", i);
		fprintf(save1, "%s\n", N2S_save(player.upgrade[i].init_cost).c_str());

		fprintf(save1, "upg_scale %d\n", i);
		fprintf(save1, "%s\n", N2S_save(player.upgrade[i].scale).c_str());

		fprintf(save1, "upg_auto %d\n", i);
		fprintf(save1, "%s\n", N2S_save(Number(player.upgrade[i].autobuyer.enabled)).c_str());
	}

	// Save Achievements & Stories
	for (int i = 1; i <= 52; i++) {
		fprintf(save1, "ach %d\n", i);
		fprintf(save1, "%s\n", N2S_save(Number(player.ach[i])).c_str());
	}
	for (int i = 1; i <= 6; i++) {
		fprintf(save1, "story %d\n", i);
		fprintf(save1, "%s\n", N2S_save(Number(player.story[i])).c_str());
	}

	// Save Pre-Alpha State
	fprintf(save1, "score\n%s\n", N2S_save(player.pre_alpha.score).c_str());
	for (int i = 0; i < 8; ++i) {
		fprintf(save1, "amount %d\n%s\n", i, N2S_save(player.pre_alpha.amount[i]).c_str());
	}
	fprintf(save1, "highest\n%s\n", N2S_save(player.pre_alpha.highest).c_str());

	// Save Instabilities
	fprintf(save1, "GIM\n%s\n", N2S_save(player.GIM).c_str());
	fprintf(save1, "PIM\n%s\n", N2S_save(player.PIM).c_str());
	fprintf(save1, "PF\n%s\n", N2S_save(player.PF).c_str());

	// Save Time & Tabs
	fprintf(save1, "last_tick\n%s\n", N2S_save(Number(player.last_tick)).c_str());
	fprintf(save1, "currenttab\n%s\n", N2S_save(Number(player.currenttab)).c_str());
	fprintf(save1, "unlockedtabs\n%s\n", N2S_save(Number(player.unlockedtabs)).c_str());
	fprintf(save1, "unlocked_prestige\n%s\n", N2S_save(Number(player.unlocked_prestige)).c_str());

	// Save Time Flux
	fprintf(save1, "time_flux\n%s\n", N2S_save(Number(player.time_flux)).c_str());
	fprintf(save1, "flux_pool\n%s\n", N2S_save(Number(flux_pool)).c_str());
	fprintf(save1, "flux_elapsed\n%s\n", N2S_save(Number(flux_elapsed)).c_str());

	// Save Alpha State
	fprintf(save1, "unlocked_alpha\n%s\n", N2S_save(Number(player.unlocked_alpha)).c_str());
	fprintf(save1, "unlocked_simulator\n%s\n", N2S_save(Number(player.alpha.unlocked_simulator)).c_str());
	// Persist the Alpha Simulator ball states: value / x / y / spawn (life) each.
	for (int i = 0; i < 20; ++i) {
		fprintf(save1, "sim_ball %d\n", i);
		fprintf(save1, "%s %s %s %s\n",
		        N2S_save(player.alpha.simulator.ball[i].value).c_str(),
		        N2S_save(player.alpha.simulator.ball[i].x).c_str(),
		        N2S_save(player.alpha.simulator.ball[i].y).c_str(),
		        N2S_save(player.alpha.simulator.ball[i].spawn).c_str());
	}

	// ---- Persist Alpha Simulator progress (level / score / accumulate / drops / upgrade levels) ----
	fprintf(save1, "sim_level\n%s\n", N2S_save(player.alpha.simulator.level).c_str());
	fprintf(save1, "sim_score\n%s\n", N2S_save(player.alpha.simulator.score).c_str());
	fprintf(save1, "sim_accumulate\n%s\n", N2S_save(player.alpha.simulator.accumulate).c_str());
	for (int t = 1; t <= 100; t++) {
		if (player.alpha.simulator.drops[t] > ZERO)
			fprintf(save1, "sim_drop_%d\n%s\n", t, N2S_save(player.alpha.simulator.drops[t]).c_str());
	}
	for (int i = 1; i <= 100; i++)
		for (int j = 0; j <= 3; j++)
			if (player.alpha.simulator.level_drops[i][j].level > ZERO)
				fprintf(save1, "sim_ld_%d_%d\n%s\n", i, j, N2S_save(player.alpha.simulator.level_drops[i][j].level).c_str());
	for (int a = 0; a <= 1; a++)
		for (int j = 0; j <= 2; j++)
			if (player.alpha.simulator.level_global[a][j].level > ZERO)
				fprintf(save1, "sim_lg_%d_%d\n%s\n", a, j, N2S_save(player.alpha.simulator.level_global[a][j].level).c_str());
	fprintf(save1, "break_alpha\n%s\n", N2S_save(Number(player.break_alpha)).c_str());
	fprintf(save1, "alpha_point\n%s\n", N2S_save(player.alpha.point).c_str());
	fprintf(save1, "alpha_count\n%s\n", N2S_save(player.alpha.alpha).c_str());
	fprintf(save1, "alpha_highest\n%s\n", N2S_save(player.alpha.highest).c_str());
	fprintf(save1, "alpha_tab\n%s\n", N2S_save(Number(player.alpha.tab)).c_str());
	fprintf(save1, "alpha_auto\n%s\n", N2S_save(Number(player.alpha.autobuyer)).c_str());
	fprintf(save1, "fastest_ap\n%s\n", N2S_save(player.fastest_ap_per_min).c_str());

	fprintf(save1, "alpha_score\n%s\n", N2S_save(player.alpha.score).c_str());
	fprintf(save1, "alpha_am_best\n%s\n", N2S_save(player.alpha.am_best).c_str());
	fprintf(save1, "alpha_am_rate\n%s\n", N2S_save(player.alpha.am_rate).c_str());
	fprintf(save1, "alpha_AIM\n%s\n", N2S_save(player.alpha.AIM).c_str());
	for (int i = 0; i < 8; ++i) {
		fprintf(save1, "alpha_amount %d\n%s\n", i, N2S_save(player.alpha.amount[i]).c_str());
	}

	// Save Beta game state.
	fprintf(save1, "unlocked_beta\n%s\n", N2S_save(Number(player.unlocked_beta)).c_str());
	fprintf(save1, "beta_point\n%s\n", N2S_save(player.beta.point).c_str());
	fprintf(save1, "beta_count\n%s\n", N2S_save(player.beta.beta).c_str());
	fprintf(save1, "beta_highest\n%s\n", N2S_save(player.beta.highest).c_str());
	fprintf(save1, "beta_tab\n%s\n", N2S_save(Number(player.beta.tab)).c_str());
	fprintf(save1, "beta_auto\n%s\n", N2S_save(Number(player.beta.autobuyer)).c_str());
	fprintf(save1, "fastest_bp\n%s\n", N2S_save(player.fastest_bp_per_min).c_str());

	fprintf(save1, "beta_score\n%s\n", N2S_save(player.beta.score).c_str());
	fprintf(save1, "beta_bm_best\n%s\n", N2S_save(player.beta.bm_best).c_str());
	fprintf(save1, "beta_bm_rate\n%s\n", N2S_save(player.beta.bm_rate).c_str());
	fprintf(save1, "beta_BIM\n%s\n", N2S_save(player.beta.BIM).c_str());
	fprintf(save1, "beta_highest_am\n%s\n", N2S_save(player.beta.highest_am).c_str());
	for (int i = 0; i < 8; ++i) {
		fprintf(save1, "beta_amount %d\n%s\n", i, N2S_save(player.beta.amount[i]).c_str());
	}
	for (int i = 0; i < 4; ++i) {
		fprintf(save1, "beta_weight %d\n%s\n", i, N2S_save(player.beta.weights[i]).c_str());
	}
	if (!flush_and_close(save1)) {
		remove(tmp.c_str());
		return;
	}

	if (!atomic_replace_file(tmp, target)) {
		copy_file_contents(tmp, target);
		remove(tmp.c_str());
	}
}

void load_game_state(const string& filename, player_& player) {
	FILE* save1 = fopen(filename.c_str(), "r");
	if (!save1) {
		// If save file doesn't exist, try backup or tmp
		string tmp = filename + ".tmp";
		string bak = filename + ".backup";
		FILE* t = fopen(tmp.c_str(), "r");
		if (t) {
			fclose(t);
			atomic_replace_file(tmp, filename);
			save1 = fopen(filename.c_str(), "r");
		} else {
			FILE* b = fopen(bak.c_str(), "r");
			if (b) {
				fclose(b);
				atomic_replace_file(bak, filename);
				save1 = fopen(filename.c_str(), "r");
			}
		}
		if (!save1) {
			init(player); // Initialize new game if no save exists
			return;
		}
	}

	char buffer[1024];
	auto read_line = [&]() -> std::string {
		if (fgets(buffer, sizeof(buffer), save1) == nullptr)
			return "";
		std::string line = buffer;
		while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
			line.pop_back();
		return line;
	};

	try {
		string line = read_line();
		while (line != "") {
			if (line.substr(0, 9) == "upg_level") {
				int id = atoi(line.substr(10).c_str());
				string val_ = read_line();
				if(id < 1000) player.upgrade[id].level = parse_Number(val_);
			} else if (line.substr(0, 8) == "upg_cost") {
				int id = atoi(line.substr(9).c_str());
				string val_ = read_line();
				if(id < 1000) player.upgrade[id].init_cost = parse_Number(val_);
			} else if (line.substr(0, 9) == "upg_scale") {
				int id = atoi(line.substr(10).c_str());
				string val_ = read_line();
				if(id < 1000) player.upgrade[id].scale = parse_Number(val_);
			} else if (line.substr(0, 8) == "upg_auto") {
				int id = atoi(line.substr(9).c_str());
				string val_ = read_line();
				if(id < 1000) player.upgrade[id].autobuyer.enabled = (parse_Number(val_)._to_long_double() > 0.5);
			} else if (line == "score") {
				string val_ = read_line();
				player.pre_alpha.score = parse_Number(val_);
			} else if (line.substr(0, 6) == "amount") {
				int id = atoi(line.substr(7).c_str());
				string val_ = read_line();
				if(id < 8) player.pre_alpha.amount[id] = parse_Number(val_);
			} else if (line == "highest") {
				string val_ = read_line();
				player.pre_alpha.highest = parse_Number(val_);
			} else if (line == "GIM") {
				string val_ = read_line();
				player.GIM = parse_Number(val_);
			} else if (line == "PIM") {
				string val_ = read_line();
				player.PIM = parse_Number(val_);
			} else if (line == "PF") {
				string val_ = read_line();
				player.PF = parse_Number(val_);
			} else if (line == "last_tick") {
				string val_ = read_line();
				long double t = parse_Number(val_)._to_long_double();
				long double time = (_get_time() - t) / 1000.0;
				if (time >= 0) player.time_flux += time;
				player.last_tick = t;
			} else if (line == "currenttab") {
				string val_ = read_line();
				player.currenttab = static_cast<int>(parse_Number(val_)._to_long_double());
			} else if (line == "unlockedtabs") {
				string val_ = read_line();
				player.unlockedtabs = static_cast<int>(parse_Number(val_)._to_long_double());
			} else if (line == "unlocked_prestige") {
				string val_ = read_line();
				player.unlocked_prestige = (parse_Number(val_)._to_long_double() > 0.5);
			} else if (line == "time_flux") {
				string val_ = read_line();
				player.time_flux += parse_Number(val_)._to_long_double();
			} else if (line == "flux_pool") {
				string val_ = read_line();
				flux_pool = parse_Number(val_);
			} else if (line == "flux_elapsed") {
				string val_ = read_line();
				flux_elapsed = parse_Number(val_)._to_long_double();
			} else if (line == "unlocked_alpha") {
				string val_ = read_line();
				player.unlocked_alpha = (parse_Number(val_)._to_long_double() > 0.5);
			} else if (line == "break_alpha") {
				string val_ = read_line();
				player.break_alpha = (parse_Number(val_)._to_long_double() > 0.5);
			} else if (line == "alpha_point") {
				string val_ = read_line();
				player.alpha.point = parse_Number(val_);
			} else if (line == "alpha_count") {
				string val_ = read_line();
				player.alpha.alpha = parse_Number(val_);
			} else if (line == "alpha_highest") {
				string val_ = read_line();
				player.alpha.highest = parse_Number(val_);
			} else if (line == "alpha_tab") {
				string val_ = read_line();
				player.alpha.tab = static_cast<int>(parse_Number(val_)._to_long_double());
			} else if (line.substr(0, 4) == "ach ") {
				int id = atoi(line.substr(4).c_str());
				string val_ = read_line();
				if (id < 1000) player.ach[id] = (parse_Number(val_)._to_long_double() > 0.5);
			} else if (line.substr(0, 6) == "story ") {
				int id = atoi(line.substr(6).c_str());
				string val_ = read_line();
				if (id < 1000) player.story[id] = (parse_Number(val_)._to_long_double() > 0.5);
			} else if (line == "alpha_auto") {
				string val_ = read_line();
				player.alpha.autobuyer = (parse_Number(val_)._to_long_double() > 0.5);
			} else if (line == "fastest_ap") {
				string val_ = read_line();
				player.fastest_ap_per_min = parse_Number(val_);
			} else if (line == "alpha_score") {
				string val_ = read_line();
				player.alpha.score = parse_Number(val_);
			} else if (line == "alpha_am_best") {
				string val_ = read_line();
				player.alpha.am_best = parse_Number(val_);
			} else if (line == "alpha_am_rate") {
				string val_ = read_line();
				player.alpha.am_rate = parse_Number(val_);
			} else if (line == "alpha_AIM") {
				string val_ = read_line();
				player.alpha.AIM = parse_Number(val_);
			} else if (line.substr(0, 12) == "alpha_amount") {
				int id = atoi(line.substr(13).c_str());
				string val_ = read_line();
				if(id < 8) player.alpha.amount[id] = parse_Number(val_);
			} else if (line == "beta_auto") {
				string val_ = read_line();
				player.beta.autobuyer = (parse_Number(val_)._to_long_double() > 0.5);
			} else if (line == "beta_score") {
				string val_ = read_line();
				player.beta.score = parse_Number(val_);
			} else if (line == "unlocked_beta") {
				string val_ = read_line();
				player.unlocked_beta = (parse_Number(val_)._to_long_double() > 0.5);
			} else if (line == "beta_point") {
				string val_ = read_line();
				player.beta.point = parse_Number(val_);
			} else if (line == "beta_count") {
				string val_ = read_line();
				player.beta.beta = parse_Number(val_);
			} else if (line == "beta_highest") {
				string val_ = read_line();
				player.beta.highest = parse_Number(val_);
			} else if (line == "beta_highest_am") {
				string val_ = read_line();
				player.beta.highest_am = parse_Number(val_);
			} else if (line == "beta_tab") {
				string val_ = read_line();
				player.beta.tab = static_cast<int>(parse_Number(val_)._to_long_double());
			} else if (line == "fastest_bp") {
				string val_ = read_line();
				player.fastest_bp_per_min = parse_Number(val_);
			} else if (line == "beta_bm_best") {
				string val_ = read_line();
				player.beta.bm_best = parse_Number(val_);
			} else if (line == "beta_bm_rate") {
				string val_ = read_line();
				player.beta.bm_rate = parse_Number(val_);
			} else if (line == "beta_BIM") {
				string val_ = read_line();
				player.beta.BIM = parse_Number(val_);
			} else if (line.substr(0, 11) == "beta_amount") {
				int id = atoi(line.substr(12).c_str());
				string val_ = read_line();
				if(id < 8) player.beta.amount[id] = parse_Number(val_);
			} else if (line.substr(0, 11) == "beta_weight") {
				int id = atoi(line.substr(12).c_str());
				string val_ = read_line();
				if (id >= 0 && id < 4) player.beta.weights[id] = parse_Number(val_);

			} else if (line == "sim_level") {
				string val_ = read_line();
				player.alpha.simulator.level = parse_Number(val_);
			} else if (line == "sim_score") {
				string val_ = read_line();
				player.alpha.simulator.score = parse_Number(val_);
			} else if (line == "sim_accumulate") {
				string val_ = read_line();
				player.alpha.simulator.accumulate = parse_Number(val_);
			} else if (line.substr(0, 9) == "sim_drop_") {
				int id = atoi(line.substr(9).c_str());
				string val_ = read_line();
				if (id >= 1 && id <= 100) player.alpha.simulator.drops[id] = parse_Number(val_);
			} else if (line.substr(0, 7) == "sim_ld_") {
				int i2 = 0, j2 = 0;
				sscanf(line.c_str(), "sim_ld_%d_%d", &i2, &j2);
				string val_ = read_line();
				if (i2 >= 1 && i2 <= 100 && j2 >= 0 && j2 <= 3) player.alpha.simulator.level_drops[i2][j2].level = parse_Number(val_);
			} else if (line.substr(0, 7) == "sim_lg_") {
				int a2 = 0, j2 = 0;
				sscanf(line.c_str(), "sim_lg_%d_%d", &a2, &j2);
				string val_ = read_line();
				if (a2 >= 0 && a2 <= 1 && j2 >= 0 && j2 <= 2) player.alpha.simulator.level_global[a2][j2].level = parse_Number(val_);
			} else if (line.substr(0, 8) == "sim_ball") {
				int id = atoi(line.substr(9).c_str());
				string val_ = read_line();
				if (id >= 0 && id < 20) {
					std::istringstream iss(val_);
					string t0, t1, t2, t3;
					iss >> t0 >> t1 >> t2 >> t3;
					if (!t0.empty()) player.alpha.simulator.ball[id].value = parse_Number(t0);
					if (!t1.empty()) player.alpha.simulator.ball[id].x     = parse_Number(t1);
					if (!t2.empty()) player.alpha.simulator.ball[id].y     = parse_Number(t2);
					if (!t3.empty()) player.alpha.simulator.ball[id].spawn = parse_Number(t3);
				}
			}


			line = read_line();
		}
	} catch (...) {
		fclose(save1);
		init(player); // Reset on error
		return;
	}

	float w[4];
	for (int i = 0; i < 4; ++i) {
		w[i] = (float)player.beta.weights[i]._to_long_double();
		if (w[i] < -1.0f) w[i] = -1.0f;
		if (w[i] >  1.0f) w[i] =  1.0f;
	}
	float sum = w[0] + w[1] + w[2] + w[3];
	if (sum < -1.0f) {
		float deficit = (-1 * player._get_BE() - sum) / 4.0f;
		for (int i = 0; i < 4; ++i) {
			w[i] += deficit;
			if (w[i] > 1.0f) w[i] = 1.0f;
		}
	}
	for (int i = 0; i < 4; ++i) player.beta.weights[i] = (long double)w[i];

	player.last_tick = _get_time();
	cost_init(player); // Re-initialize costs based on loaded upgrades
	player.alpha.simulator.cost_init();
	player.alpha.simulator.am_ref = &player.alpha.score; // Am for Global(Am) buys
	player.alpha.simulator.ap_ref = &player.alpha.point; // AP for Global(AP) buys
	fclose(save1);
}

// Unified Read Function
void read() {
	// 1. Load Options first, because they determine the save file name
	load_options(player);

	// 2. Load Game State using the filename from options
	string savefile = player.options[3].val_s;
	if (savefile.empty()) savefile = "save1.txt";

	load_game_state(savefile, player);
}

// Unified Save Function
void save() {
	save_options(player);
	save_game_state(player);
}

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType) {
	switch (dwCtrlType) {
		case CTRL_C_EVENT: //CTRL + C
		case CTRL_BREAK_EVENT: //CTRL + BREAK
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			save();
			stop_bgm();
			break;
	}

	return 0;
}


void loop() {
	long double last_save_tick = _get_time();
	while (true) {
		MSG msg;
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		long double tick = _get_time();
		long double interval = tick - player.last_tick;

		if (interval < 0) interval = 0;
		if (interval > 1000.0L) interval = 1000.0L;   // Clamp a stall/breakpoint spike so one frame cannot inject a huge delta into production/time.

		player.last_tick = tick;

		update(player, interval / 1000);

		Sleep(20);

#ifdef _WIN32
		if (g_use_gui) {
			if (!g_gui_running) break;
			GUI_RenderFrame(player);
		}
#endif

		if (tick - last_save_tick > 10000) {
			cost_init(player);
			save();
			last_save_tick = tick;
		}
	}
}

int main() {
	// Enable console output even in GUI mode for debugging
	/*
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONIN$", "r", stdin);
	*/
	log_init();
	cost_init(player);

#ifdef _WIN32
	g_use_gui = true;
	if (!GUI_Init()) {
		cout << "[ERROR] GUI Init failure. Falling back to Console mode." << endl;
		g_use_gui = false;
		system("pause");
	}
#endif

	cout << "=== Starting Project Simulation ===" << endl;

	try {
		read(); // This calls load_options() then load_game_state()
	} catch (const std::exception& e) {
		std::cerr << "[ERROR] read() threw: " << e.what() << std::endl;
	} catch (...) {
		std::cerr << "[ERROR] read() threw unknown exception" << std::endl;
	}
	cout << "=== Game Loop Starting ===" << endl;
	loop();

#ifdef _WIN32
	if (g_use_gui) GUI_Shutdown();
#endif

	//FreeConsole();
	return 0;
}
