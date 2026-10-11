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
	    Section 2 - Number class and helper initialization.
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
// Number
// Number allows a much greater number to be stored.
// layer = 0: x = val.
// layer = 1: x = 10^(val)
// layer = 2: x = 10^(10^val), etc.
// sign = false: 1/x.
// neg = -1: negative number.
// Construction: _to_Number(x).
// ----------------------------------------------------------------------
class Number {
	public:
		int         neg   = 1;
		bool        sign  = true;
		uint64_t    layer = 0;
		long double val   = 0.0L;

		// ---------- Construction ----------
		constexpr Number() : neg(1), sign(true), layer(0), val(0.0L) {}

		Number(long double v) : neg(1), sign(true), layer(0), val(v) {
			normalize_inplace();
		}
		Number(long double v, long double l)
			: neg(1), sign(true), layer((uint64_t)l), val(v) {
			normalize_inplace();
		}
		Number(bool s, long double v, long double l)
			: neg(1), sign(s), layer((uint64_t)l), val(v) {
			normalize_inplace();
		}
		Number(bool s, int n, long double v, long double l)
			: neg(n < 0 ? -1 : 1), sign(s), layer((uint64_t)l), val(v) {
			normalize_inplace();
		}
		Number(int n, long double v, long double l)
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

		[[nodiscard]] Number normalize() const {
			Number n = *this;
			n.normalize_inplace();
			return n;
		}

		// ---------- normalize ----------
		void normalize_inplace();

		// ---------- Comparison ----------
		static int cmp_abs(const Number& a, const Number& b);
		std::partial_ordering operator<=>(const Number& other) const;
		bool operator==(const Number& other) const;

		// ---------- Operators ----------
		Number operator+(const Number& other) const;
		Number operator-(const Number& other) const;
		Number operator*(const Number& other) const;
		Number operator/(const Number& other) const;

		Number& operator+=(const Number& other) {
			*this = *this + other;
			return *this;
		}
		Number& operator-=(const Number& other) {
			*this = *this - other;
			return *this;
		}
		Number& operator*=(const Number& other) {
			*this = *this * other;
			return *this;
		}
		Number& operator/=(const Number& other) {
			*this = *this / other;
			return *this;
		}

		// ---------- Helper ----------
		static Number invert(const Number& x) {
			Number r = x.normalize();
			r.sign = !r.sign;
			return r;
		}
		static Number log10_of(const Number& x);
		static Number pow10_of(const Number& x);

		[[nodiscard]] Number pow(const Number& exp) const;
		[[nodiscard]] Number log(const Number& other) const;
		[[nodiscard]] Number pow_sum(const Number& other) const;

		[[nodiscard]] Number dilate(const Number& exp) const;
		[[nodiscard]] Number tetrate(const Number& exp) const;
		[[nodiscard]] Number tetrate(const Number& exp, const Number& layer) const;

		// ---------- Transform ----------
		long double _to_long_double() const;
		[[nodiscard]] std::string toString() const;

		friend std::ostream& operator<<(std::ostream& os, const Number& n) {
			os << n.toString();
			return os;
		}
};

const Number _ZERO = Number(0.0L);
const Number _ONE = Number(1.0L);
const Number _TWO = Number(2.0L);
const Number _FIVE = Number(5.0L);
const Number _TEN = Number(10.0L);

// Helper function for normalize().
void Number::normalize_inplace() {
	if (std::isnan(val)) {
		*this = Number();
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

// Converts any kind of arithmetic (std::is_arithmetic_v<T>) numbers
// into Number.
template<typename T>
Number _to_Number(T x) {
	if constexpr (std::is_same_v<T, Number>) return x;
	else if constexpr (std::is_arithmetic_v<T>) {
		Number n;
		if (x == 0 || std::isnan(x)) return n;
		if (x < 0) {
			n.neg = -1;
			x = -x;
		}
		if (std::isinf(x)) {
			n.layer = UINT64_MAX;
			n.val = 10.0L;
			return n;
		}
		if (x < 1) {
			n.sign = false;
			x = 1.0L / x;
		}
		n.layer = 0;
		n.val   = x;
		while (n.val >= 10.0L) {
			n.val = log10l(n.val);
			n.layer++;
		}
		return n;
	} else {
		return Number(true, 0, 0);
	}
}

// Comparing absolute value.
int Number::cmp_abs(const Number& a, const Number& b) {
	if (a.sign != b.sign) return a.sign ? 1 : -1;
	int c;
	if      (a.layer != b.layer) c = (a.layer > b.layer) ?  1 : -1;
	else if (a.val   != b.val)   c = (a.val   > b.val)   ?  1 : -1;
	else                         c = 0;
	return a.sign ? c : -c;
}

std::partial_ordering Number::operator<=>(const Number& other) const {
	const Number& a = *this;
	const Number& b = other;

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

bool Number::operator==(const Number& other) const {
	Number a = normalize(), b = other.normalize();
	if (a.is_zero() && b.is_zero()) return true;
	return a.neg == b.neg && a.sign == b.sign
	       && a.layer == b.layer && a.val == b.val;
}

static Number add_pos_ge1(const Number& a, const Number& b) {
	if (a.layer == 0 && b.layer == 0) {
		Number r;
		r.val = a.val + b.val;
		r.normalize_inplace();
		return r;
	}
	auto to_exp = [](const Number& x) -> long double {
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
		Number r;
		r.layer = 1;
		r.val = nl;
		r.normalize_inplace();
		return r;
	}

	Number La = Number::log10_of(a);
	Number Lb = Number::log10_of(b);
	bool a_ge_b = (La >= Lb);
	Number L_max = a_ge_b ? La : Lb;
	Number L_min = a_ge_b ? Lb : La;
	Number L_diff = L_max - L_min;
	if (L_diff >= _to_Number(18)) return a_ge_b ? a : b;
	if (L_max.layer >= 2)         return a_ge_b ? a : b;

	Number t = Number::pow10_of(_ZERO - L_diff);
	Number correction = Number::log10_of(t + _ONE);
	return Number::pow10_of(L_max + correction);
}

static long double to_ld(const Number& x) {
	Number x0 = x.normalize();
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

static Number add_same_sign(const Number& a, const Number& b) {
	Number aa = a, bb = b;
	aa.neg = 1;
	bb.neg = 1;
	int sgn = a.neg;

	if (aa.sign && bb.sign) {
		Number r = add_pos_ge1(aa, bb);
		r.neg = sgn;
		return r;
	}
	if (!aa.sign && !bb.sign) {
		long double va = to_ld(aa);
		long double vb = to_ld(bb);
		Number r;
		r.val = va + vb;
		r.normalize_inplace();
		r.neg = sgn;
		return r;
	}

	long double va = to_ld(aa);
	long double vb = to_ld(bb);
	if (va < vb) std::swap(va, vb);
	if (vb < va * 1e-18L) {
		Number r = (va == to_ld(aa)) ? aa : bb;
		r.neg = sgn;
		return r;
	}
	Number r;
	r.val = va + vb;
	r.normalize_inplace();
	r.neg = sgn;
	return r;
}

static Number sub_same_sign(const Number& a, const Number& b) {
	Number aa = a, bb = b;
	aa.neg = 1;
	bb.neg = 1;
	int sgn = a.neg;

	long double va = to_ld(aa);
	long double vb = to_ld(bb);
	if (vb == 0.0L) {
		Number r = aa;
		r.neg = sgn;
		return r;
	}

	if (vb < va * 1e-18L) {
		Number r = aa;
		r.neg = sgn;
		return r;
	}

	Number r;
	r.val = va - vb;
	r.normalize_inplace();
	r.neg = sgn;
	return r;
}

Number Number::operator+(const Number& other) const {
	Number a = normalize(), b = other.normalize();
	if (a.is_zero()) return b;
	if (b.is_zero()) return a;

	if (a.neg == b.neg) {
		return add_same_sign(a, b);
	}
	int c = cmp_abs(a, b);
	if (c == 0) return Number();
	if (c > 0) {
		Number r = sub_same_sign(a, b);
		r.neg = a.neg;
		return r;
	} else {
		Number r = sub_same_sign(b, a);
		r.neg = b.neg;
		return r;
	}
}

Number Number::operator-(const Number& other) const {
	Number b = other.normalize();
	b.neg = -b.neg;
	return (*this) + b;
}

// log10(x)£ºReduce layer by 1, log10 if layer = 0.
Number Number::log10_of(const Number& x) {
	Number r = x.normalize();
	if (r.is_zero() || r.neg < 0) return Number();

	Number L;
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

Number Number::pow10_of(const Number& x) {
	Number y = x.normalize();
	if (y.is_zero()) return Number(1.0L);

	if (y.layer >= 2) {
		if (!y.sign) return Number(1.0L);
		Number out;
		out.neg = 1;
		out.sign = true;
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
				Number out;
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
		Number out;
		out.neg = 1;
		out.sign = true;
		out.layer = 1;
		out.val = y_ld;
		out.normalize_inplace();
		return out;
	}
	Number out;
	out.val = r;
	out.normalize_inplace();
	return out;
}

Number Number::operator*(const Number& other) const {
    Number a = normalize(), b = other.normalize();
    if (a.is_zero() || b.is_zero()) return Number();

    int rn = a.neg * b.neg;

    a.neg = 1;
    b.neg = 1;

    Number la = log10_of(a);
    Number lb = log10_of(b);
    Number s  = la + lb;
    Number r  = pow10_of(s);

    r.neg = rn;
    return r.normalize();
}

Number Number::operator/(const Number& other) const {
	Number b = other.normalize();
	if (b.is_zero()) return Number();
	return (*this) * invert(b);
}

Number Number::pow(const Number& exponent) const {
	Number b = normalize();
	Number e = exponent.normalize();
	if (b.is_zero()) return Number();
	if (e.is_zero()) return Number(1.0L);
	if (b.neg < 0)   return Number();

	// x^y = 10^(log10(x) * y)
	Number lb = log10_of(b);
	Number le = lb * e;
	return pow10_of(le);
}

Number Number::log(const Number& base) const {
	Number a = normalize(), b = base.normalize();
	if (a.is_zero() || b.is_one() || b.is_zero()) return Number();
	if (a.neg < 0 || b.neg < 0) return Number();
	return log10_of(a) / log10_of(b);
}

Number Number::pow_sum(const Number& other) const {
	Number nn = this->normalize();
	Number ss = other.normalize();
	if (nn.neg < 0 || ss.is_zero()) return Number(1.0L);
	if (nn.is_zero() || nn.is_one()) return Number(1.0L);

	Number shifted  = nn + Number(0.5L);
	Number exp_one  = ss + Number(1.0L);
	if (exp_one.is_zero()) return shifted.log(Number(10.0L));

	Number result = shifted.pow(exp_one) / exp_one;
	if (result.neg < 0) result = Number();
	return result.normalize();
}

// Dilation: a^^b = 10^(log10(a)^b)
Number Number::dilate(const Number& exp) const {
	Number n = this->normalize();
	if(n <= _TEN) return n;
	return _TEN.pow(this->log(_TEN).pow(exp));
}

// Tetration: a^^^b = 10^(10^(log10(log10(a))^b))
Number Number::tetrate(const Number& exp) const {
	Number n = this->normalize();
	if(n <= _TEN.pow(_TEN)) return n;
	return _TEN.pow(_TEN.pow(this->log(_TEN).log(_TEN).pow(exp)));
}

// Tetrate: Dilation = 1, Tetration = 2, etc.
Number Number::tetrate(const Number& exp, const Number& layer) const {
	Number aa = this->normalize();
	Number a = this->normalize();
	for(Number i = _ONE; i <= layer; i += _ONE) {
		a = a.log(_TEN);
		if(a <= _ZERO) return aa;
	}
	a = a.pow(exp);
	for(Number i = _ONE; i <= layer; i += _ONE) a = _TEN.pow(a);
	return a;
}

long double Number::_to_long_double() const {
	Number a = normalize();
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

std::string Number::toString() const {
	Number a = normalize();
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
			std::string s = fmt_mantissa(m) + "e" + fmt_int(e);
			if (v < 0) s = "-" + s;
			return s;
		}
	}

	if (a.val < 15.0L) {
		long double exp0 = powl(10.0L, a.val);
		long double e    = floorl(exp0);
		long double m    = powl(10.0L, exp0 - e);
		if (m >= 9.99L) {
			m /= 10.0L;
			e += 1.0L;
		}
		if (m <  1.0L)    {
			m *= 10.0L;
			e -= 1.0L;
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

const Number inf = _to_Number(2).pow(_to_Number(1024));

Number floor(const Number& a) {
	Number x = a.normalize();

	if (x.is_zero()) return Number();

	if (x.neg < 0) {
		Number pos = x;
		pos.neg = 1;

		Number f = floor(pos);

		if (f == pos) {
			Number r = f;
			r.neg = -1;
			return r;
		}
		Number r = f + Number(1.0L);
		r.neg = -1;
		return r;
	}

	// (0, 1) -> floor = 0
	if (!x.sign) return Number();

	// layer = 0
	if (x.layer == 0) {
		if (x.val < 1e15L) {
			return Number(floorl(x.val));
		}
		return x;
	}

	// layer = 1
	if (x.layer == 1 && x.val < 15.0L) {
		long double v = powl(10.0L, x.val);
		if (std::isfinite(v) && v < 1e15L) {
			return Number(floorl(v));
		}
	}

	// layer >= 2
	return x;
}

static Number level_inc(const Number& lv) {
	if (lv.layer == 0 && lv.val < 1e15L) {
		return lv + _ONE;
	}
	Number delta = lv * _to_Number(1e-15L);
	if (delta < _ONE) delta = _ONE;
	return lv + delta;
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


static Number flux_pool = _ZERO;
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
			_accel = _ONE;
			return _ONE;
		}
		accel = 1.0L + consumed / dt;
		pool_val -= consumed;
		if (pool_val < 0) pool_val = 0;
		flux_pool = Number(true, pool_val, 0);
		flux_elapsed += dt;
	}

	_accel = _to_Number(accel);
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
Number PF = _TEN;
Number GIM = _TEN;
Number PIM = _TEN;
Number rate = _ZERO;

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
	Number level = _ZERO;
	atb autobuyer;
	Number init_cost = _ZERO;
	Number scale = _ZERO;
};

// Details in usage.
struct opt_	{
	Number val;
	string val_s;
};

// Items in pre-alpha stage.
// player.pre_alpha.highest is used to calculate GIM reduction.
struct pre_alpha_ {
	Number score = _ONE; 											// Matter.
	Number amount[8] = {_ONE,_ONE,_ONE,_ONE,_ONE,_ONE,_ONE,_ONE};	// M1-M8.
	Number highest = _ZERO;										    // For prestige->GIM.
	Number time = _to_Number(0); // How much time has passed since last prestige, etc.
};

struct alpha_ {
	Number init_second_scale = _to_Number(4); // The second scale used to calculate cost.
	Number highest = _ZERO;					  // Highest in-Alpha matter reached.
	Number point = _ZERO;					  // Alpha Points.(or AP in short.)
	Number alpha = _ZERO;					  // Alpha you have. Notice that alpha isn't AP.
	// It more represents how many times you (performed) alpha, later alpha/time may increase.
	int tab = 1;							  // Which tab you are in.
	bool autobuyer = false; 				  // Whether Alpha autobuyer (From Alpha Milestone 7, 20 Alpha) is enabled.
	Number time = _to_Number(0); // How much time has passed since last alpha, etc.

	// === Alpha Amplifier / Alpha Intensifier system ===
	Number score    = _ONE;      // Am (Alpha matter)
	Number am_best  = _ONE;      // Highest Am.
	Number am_rate  = _ZERO;     // Current Am rate.
	Number AIM      = _TEN;      // Alpha Instability Magnitude.
	Number amount[8] = {_ONE,_ONE,_ONE,_ONE,_ONE,_ONE,_ONE,_ONE}; // AM1-AM8
};

struct beta_ {
	int tab = 1;							  // Which tab you are in.
	bool autobuyer = false; 				  // Whether Beta autobuyer (NYI) is enabled.
	Number time = _to_Number(0);              // How much time has passed since last beta, etc.

	Number highest = _ZERO;					  // Highest in-Beta matter reached.
	Number highest_am = _ZERO;				  // Highest in-Beta Alpha matter reached.
	Number highest_bm = _ZERO;				  // Highest in-Beta Beta matter reached.
	Number point = _ZERO;					  // Beta Points.
	Number beta = _ZERO;					  // Beta you have.

	Number decay     = _to_Number(0.99);      // The base Beta Decay Number (BDN).
	Number decay_factor = _to_Number(0.5);    // The Beta Decay Factor (BDF).
	Number score     = _ONE;                  // Bm (Beta matter)
	Number bm_best  = _ONE;                   // Highest Bm.
	Number bm_rate  = _ZERO;                  // Current Bm rate.
	Number BIM      = _TEN;                   // Beta Instability Magnitude.
	Number amount[8] = {_ONE,_ONE,_ONE,_ONE,_ONE,_ONE,_ONE,_ONE};
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
const Number am[100] = {_ONE, _to_Number(2), _to_Number(4), _to_Number(8),
                        _to_Number(10), _to_Number(15), _to_Number(20),
                       };
const string am_name[100] = {
	" Alpha: Buying Aeromagnifiers no longer resets Amp. and Int.",
	" Alpha: Unlock Autobuyers for Amplifiers.",
	" Alpha: Unlock Autobuyers for Intensifiers.",
	" Alpha: Allow breaking Alpha (Gain AP based on matter).",
	" Alpha: Keep PUs on Alpha.",
	" Alpha: Unlock Autobuyers for Aeromagnifiers.",
	" Alpha: Unlock Autobuyer for Alpha."
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
                               "(Ach. 48) Just Enough AUP"
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
                               "Get the sixth Alpha milestone.", //Bookmark: 34 here
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
                                 "Reward: Apply Story 1 reward to\nAlpha Amplifiers.",
                                 "Reward: Multiply Alpha matter gain\nby log10(Am).",
                                 "Reward: Multiply Alpha matter gain\nby log10(AP).",
                                 //41-
                                 "Reward: x10 to AP.",
                                 "Reward: x10 to AP.",
                                 "Reward: x10 to AP.",
                                 "Reward: Apply Ach. Reward to AIs.",
                                 "Reward: ^2 to BAU8 effect.",
                                 "Reward: Buff BAU1 to x5.",
                                 "Reward: Double effect of BAU9.",
                                 "Reward: AAs gain a mult. equal to\nlog10(log10(matter))."
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
    or the seed."
};

const string story_hint[100] = {
	"",
	"...until They did it the second time.",
	"...but They did not count the crossings.",
	"...if only They had looked beneath.",
	"...the gift never asked for thanks.",
	"...until the very edge of the world."
};

const string story_reward[100] = {
	"",
	"Reward: Mult. to Amplifiers based on time spent in this Alpha.",
	"Reward: AP gain x(1 + 0.1 x Alpha).",
	"Reward: AA and AI Amplifiers gain +10% base multiplier.",
	"Reward: Am production x2.",
	"Reward: GIM reduction x2."
};
#ifdef _WIN32
#include "imgui.h"
struct Notification;
void PushNotification(const std::string& title,
                      const std::string& message,
                      const ImVec4& accent);
#endif

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
		Number fastest_ap_per_min = _ZERO;
		Number fastest_bp_per_min = _ZERO;
		std::vector<bool> ach;   // Whether Ach. x is completed.
		std::vector<bool> story; // Whether Story x is found.
		Number GIM = _TEN;
		Number PIM = _TEN;
		Number PF  = _TEN;
		long double last_tick = 0; // Used to calculate time passage.
		long double time_flux = 0; // How much time flux you have.

		Number rate_real = _ZERO;
		Number rate_real_Am = _ZERO;
		Number rate_real_Bm = _ZERO;

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
			Number cnt = _ZERO;
			for(int i = 1; i <= 50; i ++) {
				cnt += _get_ach(i) ? _ONE : _ZERO;
			}
			return cnt;
		}

		// Returns whether Story x is found.
		[[nodiscard]] bool _get_story(int id) {
			return this->story[id];
		}

		// Returns how many stories are found.
		[[nodiscard]] Number _get_story_done() {
			Number cnt = _ZERO;
			for(int i = 1; i <= 50; i ++) {
				cnt += _get_story(i) ? _ONE : _ZERO;
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
			if(id == 24 && (this->upgrade[20].level >= _to_Number(9))) {
				_unlock(id);
			}
			if(id == 25 && (this->upgrade[21].level >= _to_Number(5))) {
				_unlock(id);
			}

			//Alpha part.
			if(id == 26 && (this->unlocked_alpha)) {
				_unlock(id);
			}
			if(id == 27 && (_get_AUP_bought() > _ZERO)) {
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
			if(id == 40 && (this->alpha.score >= _to_Number(1e10))) {
				_unlock(id);
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
			if(id == 46 && (this->pre_alpha.score >= _TEN.pow(_to_Number(1e10)))) {
				_unlock(id);
			}
			if(id == 47 && (this->PIM <= _to_Number(0))) {
				_unlock(id);
			}
			if(id == 48 && (_get_AUP_bought() >= _to_Number(1e9))) {
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
			if(id == 2 && (this->alpha.alpha >= _to_Number(3))) {
				_unlock_story(id);
			}
			// Story 3: unlocked the Alpha Amplifier / Intensifier system.
			if(id == 3 && (this->upgrade[34].level.is_one())) {
				_unlock_story(id);
			}
			// Story 4: Am reached 1e10.
			if(id == 4 && (this->alpha.score >= _to_Number(1e10))) {
				_unlock_story(id);
			}
			// Story 5: Am reached the world's edge.
			if(id == 5 && (this->alpha.score >= inf)) {
				_unlock_story(id);
			}
		}

		// Unlock certain Ach. and Story if certain requirements are met.
		void _try_unlock_all() {
			for(int i = 1; i <= 50; i ++) _try_unlock(i);
			for(int i = 1; i <= 50; i ++) _try_unlock_story(i);
		}

		// Returns Ach. effect.
		// Achievements give a multiplier to:
		// id = 1: Amplifiers, x(1+0.001x)
		[[nodiscard]] Number _get_ach_effect(int id) {
			if(id == 1) {
				if(!this->ach[37]) return _ONE + _to_Number(0.001) * _get_ach_done();
				else return _ONE + _to_Number(0.005) * _get_ach_done();
			}
			return _ONE;
		}

		// Returns effect of Story x.
		// id = 1: Amplifiers, x(1+0.001s), s = game-time seconds passed.
		// id = 2: AP gain multiplier, x(1 + 0.1 * Alpha).
		// id = 3: AA/AI base multiplier, x1.1.
		// id = 4: Am production, x2.
		// id = 5: GIM reduction, x2.
		[[nodiscard]] Number _get_story_effect(int id) {
			if(id == 1) {
				Number tm = (this->alpha.time) / _to_Number(1000.0);
				return _to_Number(tm) + _ONE;
			}
			if(id == 2) {
				return _ONE + _to_Number(0.1) * this->alpha.alpha;
			}
			if(id == 3) {
				return _ONE + _to_Number(0.1);
			}
			if(id == 4) {
				return _to_Number(2);
			}
			if(id == 5) {
				return _to_Number(2);
			}
			return _ONE;
		}

		
		// --------------------------- Pre-prestige related items.
		
		// Returns current Prestige Factor, or PF.
		// It is used in the GIM -> Mult formula (10 - logPF(log10(x)) / 2).
		// It starts off at 10 and decreases as PIM decreases, which is decreased by PU3.
		// Less PF is better, but it should NEVER be less than 1.
		// ** PU3: Decrease PIM at 10% rate of GIM reduction.
		[[nodiscard]] Number _get_PF() const {
			Number init = _TEN;
			Number pim_ld = PIM;
			Number exp = pim_ld / _TEN;
			if (pim_ld >= _ZERO) init = _to_Number(8) * _to_Number(1.25).pow(exp);
			else if (pim_ld < _ZERO && PIM >= _to_Number(-100)) init = _to_Number(8) * _to_Number(1.25).pow(exp / _TEN);
			else if (pim_ld < _to_Number(-100)) init = _ONE + _to_Number(5.521852) * _to_Number(1.25).pow(exp / _TEN / _TEN);
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
			Number init = _TEN;
			Number reduction = _ZERO;
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
			Number init = _TEN;
			this->PF = _get_PF();
			Number highest_ld = this->pre_alpha.highest;
			if (((highest_ld - _ONE) < _to_Number(1e-9)) && ((_ONE - highest_ld) < _to_Number(1e-9))
			        || ((highest_ld) < _to_Number(1e-9)) && ((highest_ld) > _to_Number(-1e-9))) {
				return init;
			}
			Number tmp = (this->pre_alpha.highest + 1);
			tmp = tmp.log(_TEN);
			tmp = tmp + _ONE;
			tmp = tmp.log(this->PF);
			Number gim_red = _get_AU_effect(2) * _get_Am_effect(1);
			if (_get_story(5)) gim_red = gim_red * _get_story_effect(5);
			Number reduction = tmp / 2 * gim_red;
			tmp = reduction - init;
			if(tmp > _TEN) tmp = tmp.pow(_to_Number(0.9)) * _TEN.pow(_to_Number(0.1));
			tmp = _ZERO - tmp;
			return tmp;
		}
		
		// Returns current GIM IF you prestige at this point, used for display.
		Number _get_GIM(Number _score) {
			Number init = _TEN;
			this->PF = _get_PF();
			Number highest_ld = _score;
			if (((highest_ld - _ONE) < _to_Number(1e-9)) && ((_ONE - highest_ld) < _to_Number(1e-9))
			        || ((highest_ld) < _to_Number(1e-9)) && ((highest_ld) > _to_Number(-1e-9))) {
				return init;
			}
			Number tmp = (_score + 1);
			tmp = tmp.log(_TEN);
			tmp = tmp + _ONE;
			tmp = tmp.log(this->PF);
			Number gim_red = _get_AU_effect(2) * _get_Am_effect(1);
			if (_get_story(5)) gim_red = gim_red * _get_story_effect(5);
			Number reduction = tmp / 2 * gim_red;
			tmp = reduction - init;
			if(tmp > _TEN) tmp = tmp.pow(_to_Number(0.9)) * _TEN.pow(_to_Number(0.1));
			tmp = _ZERO - tmp;
			return tmp;
		}

		// Returns how much PIM decreases PF.
		Number _get_PIM_effect() {
			return _TEN - _get_PF();
		}

		// Returns GIM -> M^(x).
		[[nodiscard]] Number _get_GIM_effect() {
			Number gim = _get_GIM();
			if ((gim - _TEN) < _to_Number(1e-9) && (_TEN - gim) < _to_Number(1e-9)) {
				return _to_Number(0.5);
			}

			Number prod = _to_Number(11);
			prod = prod - gim;
			prod = prod.log(_to_Number(121));
			prod = (_ONE + _ONE) * prod * prod + _to_Number(0.5);
			if(prod > _to_Number(1.3225)) prod = _to_Number(1.3225).pow(_to_Number(0.75)) * prod.pow(_to_Number(0.25));
			return prod;
		}
		void _update_GIM() {
			this->GIM = _get_GIM();
			GIM = this->GIM;
		}
		
		// ---------------------------- Prestige related items.
		
		// The current PU3 effect. Work even if PU3 is not bought.
		[[nodiscard]] Number _get_PU3_effect() {
			Number init = _TEN;
			if (this->pre_alpha.highest.is_zero() || this->pre_alpha.highest.is_one()) {
				return _ZERO;
			}
			Number pf = this->PF;
			Number tmp = (this->pre_alpha.highest + 1);
			tmp = tmp.log(_TEN);
			tmp = tmp + _ONE;
			tmp = tmp.log(pf);
			Number gim_red = _get_AU_effect(2) * _get_Am_effect(1);
			if (_get_story(5)) gim_red = gim_red * _get_story_effect(5);
			Number reduction = tmp / _to_Number(2) * gim_red;
			if(reduction > _TEN) reduction = reduction.pow(_to_Number(0.9)) * _TEN.pow(_to_Number(0.1));
			reduction = reduction / _TEN;
			return reduction;
		}

		
		// Returns whether Alpha Milestone (id+1) is completed.
		bool _get_AM(int id) {
			if (this->alpha.alpha >= am[id]) return true;

			Number diff = am[id] - this->alpha.alpha;
			if (diff.layer == 0.0L && diff.sign && diff.neg > 0 &&
			        diff.val < 0.01L) {
				return true;
			}
			return false;
		}

		
		// Returns the effect of PU2, which gives a multiplier to Amp. preduction that applies BEFORE Int. effect.
		// **PU2: Give a small multiplier based on matter to Mx/s which applies before Intensifier effect.
		[[nodiscard]] Number _get_PU2_effect() {
			if(this->pre_alpha.score <= _FIVE.pow(_FIVE)) return _ONE;
			Number init = this->pre_alpha.score.log(_FIVE).log(_FIVE);
			init = _ONE + (init - _ONE) / _FIVE;
			return init;
		}

		// Used in an Ach. reward.(Ach. 33: Get the 5th Alpha Milestone.)
		[[nodiscard]] Number _get_PU2_effect(Number s) {
			if(s <= _FIVE.pow(_FIVE)) return _ONE;
			Number init = s.log(_FIVE).log(_FIVE);
			init = _ONE + (init - _ONE) / _to_Number(5);
			return init;
		}

		// Get Aeromagnifier effects.
		[[nodiscard]] Number _get_aero_effect_a() {
			Number a = (_ONE + _to_Number(0.2) * this->upgrade[20].level) * _get_AU_effect(0);
			return a;
		}
		[[nodiscard]] Number _get_aero_effect_i() {
			Number a = (_ONE + _to_Number(0.1) * this->upgrade[21].level) * _get_AU_effect(1);
			if(_get_ach(33)) a = a * _to_Number(1.01);
			if(_get_ach(34)) a = a * _to_Number(1.01);
			if(_get_ach(35)) a = a * _to_Number(1.01);
			if(_get_ach(36)) a = a * _to_Number(1.01);
			return a;
		}

        // ----------------------------------- Alpha related items.
        
		[[nodiscard]] Number _get_alpha_aero_effect_a() {
			return _ONE + _to_Number(0.2) * this->upgrade[56].level;
		}
		[[nodiscard]] Number _get_alpha_aero_effect_i() {
			Number a = _ONE + _to_Number(0.1) * this->upgrade[57].level;
			if(_get_ach(33)) a = a * _to_Number(1.01);
			if(_get_ach(34)) a = a * _to_Number(1.01);
			if(_get_ach(35)) a = a * _to_Number(1.01);
			if(_get_ach(36)) a = a * _to_Number(1.01);
			if(_get_ach(44)) a = a * _get_ach_effect(1);
			return a;
		}


		//Returns how many AP you can get with current matter. (this->alpha.highest)
		[[nodiscard]] Number _get_AP() {
			Number a = _ZERO;
			if(!this->break_alpha || this->pre_alpha.score < inf) a = _ONE;
			else a = this->pre_alpha.score.log(inf);
			a = a * _to_Number(4).pow(this->upgrade[30].level);
			if(_get_ach(30)) a = a * _to_Number(2);
			if(_get_ach(31)) a = a * _to_Number(2);
			if(_get_ach(41)) a = a * _to_Number(10);
			if(_get_ach(42)) a = a * _to_Number(10);
			if(_get_ach(43)) a = a * _to_Number(10);
			if(_get_story(2)) a = a * _get_story_effect(2);
			a = a * this->_get_BAU10_effect();
			return a;
		}

		[[nodiscard]] Number _get_AP(Number s) {
			Number a = _ZERO;
			Number b = s.normalize();
			if(!this->break_alpha || b < inf) a = _ONE;
			else a = b.log(inf);
			a = a * _to_Number(4).pow(this->upgrade[30].level);
			if(_get_ach(30)) a = a * _to_Number(2);
			if(_get_ach(31)) a = a * _to_Number(2);
			if(_get_ach(41)) a = a * _to_Number(10);
			if(_get_ach(42)) a = a * _to_Number(10);
			if(_get_ach(43)) a = a * _to_Number(10);
			if(_get_story(2)) a = a * _get_story_effect(2);
			a = a * this->_get_BAU10_effect();
			return a;
		}

		// Returns how many alpha(s) you can get if you (perform) alpha now.
		[[nodiscard]] Number _get_alpha() {
			Number a = _ONE;
			if(_get_ach(30)) a = a * _to_Number(2);

			// ** BAU3£ºMultiply Alpha gain based on matter and level.
			if (this->upgrade[32].level > _ZERO) {
				Number s = this->pre_alpha.score;
				if (s > _ONE) {
					Number log_s = s.log(_TEN);
					if (log_s > _ZERO) {
						Number x = this->upgrade[32].level;
						Number exponent = _to_Number(0.75) - _to_Number(0.75) / _to_Number(1.1).pow(x);
						if (exponent > _to_Number(0.75)) exponent = _to_Number(0.75);
						Number mul = log_s.pow(exponent);
						a = a * mul;
					}
				}
			}
			if (this->unlocked_beta) a = a * _get_Bm_effect(4);
			if (a < _ONE) return _ONE;
			return floor(a) + _to_Number(0.00001);
		}

		// Returns the cost of the next AUP purchase of a given id.
		// id = 26: with matter
		// id = 27: with Amp. Aero
		// id = 28: with Int. Aero
		// id = 29: with AP
		[[nodiscard]] Number _get_AUP_cost(int id) {
			if (id == 26) {
				Number s = _to_Number(1e100);
				Number level = this->upgrade[26].level;
				Number exp;
				if (level <= _to_Number(10)) {
					exp = level + _ONE;
				} else {
					Number extra = level - _to_Number(10);
					exp = (level + _ONE) + extra * extra;
				}

				s = _get_third_scale(s, s.pow(exp));

				return s.pow(exp);
			}
			if (id == 27) {
				Number level = this->upgrade[27].level;
				Number cost = _to_Number(5) + _to_Number(5) * level;
				if (level > _to_Number(10)) {
					Number extra = level - _to_Number(10);
					cost = cost + extra * extra;
				}
				return cost;
			}
			if (id == 28) {
				Number level = this->upgrade[28].level;
				Number cost = _to_Number(4) + _to_Number(3) * level;
				if (level > _to_Number(10)) {
					Number extra = level - _to_Number(10);
					cost = cost + extra * extra;
				}
				return cost;
			}
			if (id == 29) {
				Number s = _to_Number(2);
				Number level = this->upgrade[29].level;
				Number exp;
				if (level <= _to_Number(10)) {
					exp = level;
				} else {
					Number extra = level - _to_Number(10);
					exp = level + extra * extra;
				}
				return s.pow(exp);
			}
			return _ZERO;
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
			if(id == 0) return _to_Number(1.1).pow(this->upgrade[id + 22].level);
			if(id == 1) {
				Number level = this->upgrade[23].level;
				Number base  = _to_Number(1.02);
				const Number THRESHOLD = _to_Number(25);

				if (level <= THRESHOLD) {
					return base.pow(level);
				}

				Number extra      = level - THRESHOLD;
				Number log2_term  = (extra + _ONE).log(_to_Number(2));

				return base.pow(THRESHOLD + log2_term);
			}
			if(id == 2) {
				Number base =  _ONE +_to_Number(0.05) * this->upgrade[id + 22].level;
				if(base >= _to_Number(2)) base = (_to_Number(8) * base).pow(_to_Number(0.25));
				return base;
			}
			if(id == 3) {
				Number base = _to_Number(0.99).pow(this->upgrade[id + 22].level);
				Number x = this->upgrade[id + 22].level;
				if(base <= _to_Number(0.01)) base = (_to_Number(0.01).pow(_ONE - _ONE / x.pow(0.5)) * base.pow(_ONE / x.pow(0.5)));
				return base;
			}
			return _ONE;
		}

		// Alpha Amplifier / Alpha Intensifier related methods

		// Returns BAU effect.
		// id = 0: x4^x to AP gained.
		// id = 1: Mult. to Amplifiers based on this BAU's level and Alpha.
		Number _get_BAU_effect(int id) {
			if(id == 0) {
				if(!_get_ach(46)) return _to_Number(4).pow(this->upgrade[id + 30].level);
				else return _to_Number(5).pow(this->upgrade[id + 30].level);
			}
			if(id == 1) {
				if(this->upgrade[id + 30].level.is_zero()) return _ONE;
				Number base = _ONE + _to_Number(0.1) * this->alpha.alpha;
				return base.pow(_ONE + _to_Number(0.1) * this->upgrade[id + 30].level);
			}
			return _ONE;
		}

		// BAU8: x (log10(AP))^0.5 to Am generation
		[[nodiscard]] Number _get_BAU8_effect() {
			if (!this->upgrade[37].level.is_one()) return _ONE;
			Number ap = this->alpha.point;
			if (ap <= _ONE) return _ONE;
			Number log10_ap = ap.log(_TEN);
			if (log10_ap <= _ZERO) return _ONE;
			if(!_get_ach(45)) return log10_ap.pow(_to_Number(0.5));
			else return log10_ap;
		}

		// BAU9: -log10(AP)/100 to AIM
		[[nodiscard]] Number _get_BAU9_effect() {
			if (!this->upgrade[38].level.is_one()) return _ZERO;
			Number ap = this->alpha.point;
			if (ap <= _ONE) return _ZERO;
			Number log10_ap = ap.log(_TEN);
			if (log10_ap <= _ZERO) return _ZERO;
			if(!_get_ach(45)) return log10_ap / _to_Number(100);
			else return log10_ap / _to_Number(50);
		}

		// BAU10: x log10(Am) * level to AP gain
		[[nodiscard]] Number _get_BAU10_effect() {
			if (this->upgrade[39].level <= _ZERO) return _ONE;
			Number am = this->alpha.am_best;
			if (am <= _ONE) return _ONE;
			Number log10_am = am.log(_TEN);
			if (log10_am <= _ONE) return _ONE;
			return log10_am * this->upgrade[39].level;
		}

		[[nodiscard]] Number _get_AIM() {
			Number base = _TEN;
			if (!this->upgrade[38].level.is_zero()) base = base - this->_get_BAU9_effect();
			return base;
		}
		void _update_AIM() {
			this->alpha.AIM = _get_AIM();
		}

		[[nodiscard]] Number _get_AIM_effect() {
			_update_AIM();
			if (this->alpha.AIM == _TEN) return _to_Number(0.25);
			Number prod = _to_Number(11);
			prod = prod - this->alpha.AIM;
			prod = prod.log(_to_Number(121));
			prod = ((_ONE + _ONE) * prod * prod + _to_Number(0.5)) / _to_Number(2);
			return prod.normalize();
		}

		// Am's 4 effects, given id = 1~4£º
		//   id = 1: Mult. to GIM reduction.
		//   id = 2: Mult. to PIM reduction.
		//   id = 3: Mult. to A1-A8.
		//   id = 4: Mult. to I1-I8.
		[[nodiscard]] Number _get_Am_effect(int id) {
			if (id < 1 || id > 4) return _ONE;
			Number base = this->alpha.am_best;
			if (base <= _ONE) return _ONE;

			Number log10_base = base.log(_TEN);
			if (log10_base <= _ZERO) return _ONE;


			if (id == 1) return (_ONE + log10_base * _to_Number(0.05)).pow(_get_Bm_effect(3));
			if (id == 2) return (_ONE + log10_base * _to_Number(0.05)).pow(_get_Bm_effect(3));
			if (id == 3) return (_ONE + log10_base * _to_Number(0.1)).pow(_get_Bm_effect(3));
			if (id == 4) return (_ONE + log10_base * _to_Number(0.01)).pow(_get_Bm_effect(3));
			return _ONE;
		}

		// ------------------------------ Beta related items.
		
		[[nodiscard]] Number _get_beta_aero_effect_a() {
			Number a = _ONE + _to_Number(0.2) * this->upgrade[56].level;
			return a;
		}
		[[nodiscard]] Number _get_beta_aero_effect_i() {
			Number a = _ONE + _to_Number(0.1) * this->upgrade[57].level;
			return a;
		}

		// Returns how much BP is gained on Beta.
		[[nodiscard]] Number _get_BP() {
			if(!this->unlocked_beta) return _ONE;
			return _ONE;
		}

		[[nodiscard]] Number _get_BP(Number s) {
			if(!this->unlocked_beta) return _ONE;
			return _ONE;
		}

		[[nodiscard]] Number _get_beta() {
			if(!this->unlocked_beta) return _ONE;
			Number a = _ONE;
			a = a * _get_Bm_effect(5);
			return a;
		}

		[[nodiscard]] Number _get_BIM() {
			Number base = _TEN;
			// Placeholder.

			return base;
		}
		void _update_BIM() {
			this->beta.BIM = _get_BIM();
		}

		[[nodiscard]] Number _get_BIM_effect() {
			_update_BIM();
			if (this->beta.BIM == _TEN) return _to_Number(0.1);
			Number prod = _to_Number(11);
			prod = prod - this->alpha.AIM;
			prod = prod.log(_to_Number(121));
			prod = ((_ONE + _ONE) * prod * prod + _to_Number(0.5)) / _to_Number(5);
			return prod.normalize();
		}

		// Returns the Beta Matter (Bm) effects.
		// id = 1: Narrow gap between BDN and 1 by ^(1+sqrt(logBm)/100).
		// id = 2: Narrow gap between BDF and 1 by ^(1+sqrt(logBm)/100).
		// id = 3: Empower Am effects by ^(1+sqrt(logBm)/100).
		// id = 4: Multiply Alpha gain by logBm.
		// id = 5: Multiply Beta gain by double-log of Bm.
		[[nodiscard]] Number _get_Bm_effect(int id) const {
			if (id < 1 || id > 5) return _ONE;
			Number base = _ONE;
			if(this->beta.score > _TEN) base = _ONE + (this->beta.score).pow(0.5) / _to_Number(100);

			if (id >= 1 && id <= 3) return base;
			if (id == 4) {
				if(this->beta.score > _TEN) return this->beta.score.log(_TEN);
				return _ONE;
			}
			if (id == 5) {
				if(this->beta.score > _TEN.pow(_TEN)) return this->beta.score.log(_TEN).log(_TEN);
				return _ONE;
			}
			return _ONE;
		}

		[[nodiscard]] Number _get_BDIM() const {
			Number init = _TEN;
			// Placeholder.

			if(init < _ZERO) return _ZERO;
			return init;
		}

		[[nodiscard]] Number _get_BDN() const {
			Number init = this->beta.decay;
			// Placeholder.
			Number dif = _ONE - init;
			dif = dif.pow(_get_Bm_effect(1));
			init = _ONE - dif;
			return init;
		}

		[[nodiscard]] Number _get_BDF() const {
			Number init = this->beta.decay_factor;
			// Placeholder.
			Number dif = _ONE - init;
			dif = dif.pow(_get_Bm_effect(2));
			init = _ONE - dif;
			return init;
		}
		
		[[nodiscard]] Number _get_BD_step(Number matter) {
			if(matter <= _TEN.pow(_TEN)) return _ONE;
			else if(matter > _TEN.pow(_TEN) && matter <= _TEN.pow(_TEN.pow(_TEN))) return _to_Number(2);
			else if(matter > _TEN.pow(_TEN.pow(_TEN)) && matter <= _TEN.pow(_TEN.pow(_TEN.pow(_TEN)))) return _to_Number(3);
			else if(matter > _TEN.pow(_TEN.pow(_TEN.pow(_TEN)))/* && matter <= _TEN.pow(_TEN.pow(_TEN.pow(_TEN.pow(_TEN))))*/)
				return _to_Number(4);
			return _ONE;
		}

		[[nodiscard]] Number _get_BD_effect(Number matter, Number time) {
			if(!this->unlocked_beta) return _ONE;
			if(matter <= _TEN) return _get_BDN().pow(time);
			Number BDN = _get_BDN(), BDF = _get_BDF(), BDIM = _get_BDIM();
			Number x = matter;
			if(matter > _TEN && matter <= _TEN.pow(_TEN)) {
				x = x.log(_TEN);
			}
			else if(matter > _TEN.pow(_TEN) && matter <= _TEN.pow(_TEN.pow(_TEN))) {
				x = x.log(_TEN).log(_TEN);
			}
			else if(matter > _TEN.pow(_TEN.pow(_TEN)) && matter <= _TEN.pow(_TEN.pow(_TEN.pow(_TEN)))) {
				x = x.log(_TEN).log(_TEN).log(_TEN);
			}
			else if(matter > _TEN.pow(_TEN.pow(_TEN.pow(_TEN)))/* && matter <= _TEN.pow(_TEN.pow(_TEN.pow(_TEN.pow(_TEN))))*/) {
				x = x.log(_TEN).log(_TEN).log(_TEN).log(_TEN);
			}
			Number factor = BDN.pow((_TEN - x) / _to_Number(9)) * BDF.pow((x - _ONE) / _to_Number(9));
			factor = factor.pow(BDIM / _TEN);
			return factor.pow(time);
		}

		[[nodiscard]] Number _get_BD(Number matter, Number time) {
			if(!this->unlocked_beta) return matter;
			if(matter <= _TEN) return matter * _get_BDN().pow(time);
			Number BDN = _get_BDN(), BDF = _get_BDF(), BDIM = _get_BDIM();
			Number x = matter;
			if(matter > _TEN && matter <= _TEN.pow(_TEN)) {
				x = x.log(_TEN);
				Number factor = BDN.pow((_TEN - x) / _to_Number(9)) * BDF.pow((x - _ONE) / _to_Number(9));
				factor = factor.pow(BDIM / _TEN);
				return matter * factor.pow(time);
			}
			else if(matter > _TEN.pow(_TEN) && matter <= _TEN.pow(_TEN.pow(_TEN))) {
				x = x.log(_TEN).log(_TEN);
				Number factor = BDN.pow((_TEN - x) / _to_Number(9)) * BDF.pow((x - _ONE) / _to_Number(9));
				factor = factor.pow(BDIM / _TEN);
				return matter.pow(factor.pow(time));
			}
			else if(matter > _TEN.pow(_TEN.pow(_TEN)) && matter <= _TEN.pow(_TEN.pow(_TEN.pow(_TEN)))) {
				x = x.log(_TEN).log(_TEN).log(_TEN);
				Number factor = BDN.pow((_TEN - x) / _to_Number(9)) * BDF.pow((x - _ONE) / _to_Number(9));
				factor = factor.pow(BDIM / _TEN);
				return matter.dilate(factor.pow(time));
			}
			else if(matter > _TEN.pow(_TEN.pow(_TEN.pow(_TEN)))/* && matter <= _TEN.pow(_TEN.pow(_TEN.pow(_TEN.pow(_TEN))))*/) {
				x = x.log(_TEN).log(_TEN).log(_TEN).log(_TEN);
				Number factor = BDN.pow((_TEN - x) / _to_Number(9)) * BDF.pow((x - _ONE) / _to_Number(9));
				factor = factor.pow(BDIM / _TEN);
				return matter.tetrate(factor.pow(time));
			}
			return matter;
		}
		

		// How much matter is kept.
		[[nodiscard]] Number _get_BD_kept(Number matter) {
			return _get_BD(matter, _ONE);
		}

		
		
		// ------------------------- Upgrade related items.
		
		// Returns second scale.
		[[nodiscard]] Number _second_scale() const {
			return _to_Number(4) * _to_Number(0.99).pow(this->upgrade[25].level);
		}

		[[nodiscard]] Number _get_third_scale(const Number& s, const Number& t) const {
			static const Number THRESHOLD = _to_Number(10).pow(
			                                    _to_Number(10).pow(_to_Number(10))
			                                );

			if (t <= THRESHOLD) return s;

			Number log_s = s.log(_TEN);
			if (log_s <= _ZERO) return s;

			Number log_t = t.log(_TEN);
			if (log_t <= _ONE) return s;

			Number log_log_t = log_t.log(_TEN);
			if (log_log_t <= _ZERO) return s;

			Number k = log_log_t / _to_Number(10);

			Number exponent = (log_s < _ONE) ? (_ONE / k) : k;

			Number new_log_s = log_s.pow(exponent);

			if (new_log_s < log_s) new_log_s = log_s;

			if (new_log_s < _to_Number(1e-6L)) new_log_s = _to_Number(1e-6L);

			return _to_Number(10).pow(new_log_s);
		}

		// Returns cost scale with given highest value. (this->alpha.highest)
		// ** NOTE: May later be discarded.
		[[nodiscard]] Number _get_scale(int id) {
			if (this->alpha.highest <= inf) return this->upgrade[id].scale;
			Number s   = this->upgrade[id].scale;
			Number emp = this->alpha.highest.log(inf);
			s = s * _second_scale().pow(emp);
			s = _ONE + (s - _ONE) * _get_AU_effect(3);

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
			if (std::isnan(emp.val) || std::isnan(emp.layer) || emp.neg < 0) {
				return this->upgrade[id].scale;
			}
			Number second = _second_scale();
			if (second <= _ONE) return this->upgrade[id].scale;
			Number factor = second.pow(emp);
			if (factor.is_zero() || std::isnan(factor.val) || std::isnan(factor.layer)) {
				return this->upgrade[id].scale;
			}
			s = s * factor;
			s = _ONE + (s - _ONE) * _get_AU_effect(3);
			if (s < this->upgrade[id].scale) s = this->upgrade[id].scale;
			return s;
		}

		[[nodiscard]] Number _effective_scale(const Number& s, const Number& cost) const {
			if (cost < inf) return s;
			Number s2 = s * _second_scale().pow(cost.log(inf));
			if (cost > _TEN.pow(_TEN.pow(_TEN))) s2 = _get_third_scale(s2, cost);
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

				if (final_cost.is_zero() || std::isnan(final_cost.val) || std::isnan(final_cost.layer)) {
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
				Number exp = _to_Number(3) * _to_Number(2).pow(k);
				return _to_Number(10).pow(exp);
			}
			// BAU4
			if (id == 33) {
				Number exp = _to_Number(5) * _to_Number(2).pow(k);
				return _to_Number(10).pow(exp);
			}
			// BAU5 - 7
			if (id == 34) return _to_Number(1e6);
			if (id == 35) return _to_Number(1e10);
			if (id == 36) return _to_Number(1e15);
			if (id == 37) return _to_Number(1e20);
			if (id == 38) return _to_Number(1e25);
			if (id == 39) {
				Number i   = this->upgrade[id].init_cost;
				Number exp = k;
				return i * this->upgrade[id].scale.pow(exp);
			}

			// Alpha Amplifiers (40-47) / Alpha Intensifiers (48-55)
			// Alpha Aeromagnifiers (56, 57)
			if (40 <= id && id < 58) {
				Number i   = this->upgrade[id].init_cost;
				Number exp = k * k / 2 + k / 2;
				Number cost = i * this->upgrade[id].scale.pow(exp);
				Number s   = _get_scale(id, cost);
				Number final_cost = i * s.pow(exp);

				if (final_cost.is_zero() || std::isnan(final_cost.val) || std::isnan(final_cost.layer)) {
					final_cost = i * this->upgrade[id].scale.pow(exp);
				}

				s = _get_third_scale(s, final_cost);
				return i * s.pow(exp);
			}

			return _ZERO;
		}

		// Approximate level M such that cumulative cost from current level x
		// up to M-1 approximately equals total resource T.
		// Formula based on cost(k) = init_cost * scale^(k(k+1)/2):
		//    M ~ (1 + sqrt(1 + 8R)) / 2,   R = log_scale(T / init_cost)
		// When cost > inf, the effective scale is larger; iterate to convergence.
		[[nodiscard]] Number _bulk_approx_level(int id, const Number& x, const Number& T) {
			if (T < _level_cost(id, x)) return x;

			Number lo = x;
			Number hi = x + _to_Number(2);

			for (int rep = 0; rep < 60; ++rep) {
				if (_level_cost(id, hi) > T) break;
				Number span = hi - x;
				if (span <= _ZERO) span = _ONE;
				hi = x + span * _to_Number(2);
				if (hi <= x) hi = x * _to_Number(2);
			}

			for (int iter = 0; iter < 100; ++iter) {
				Number mid = (lo + hi) / _to_Number(2);
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
				if(this->_get_AUP_avaliable() >= _ONE) this->upgrade[id].level = level_inc(this->upgrade[id].level);
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
			// Alpha Aeromagnifiers (56, 57), consumed with Am
			if (40 <= id && id < 58) {
				Number l    = this->upgrade[id].level;
				Number cost = _level_cost(id, l);
				if (this->alpha.score >= cost) {
					this->alpha.score -= cost;
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
				if (total <= _ZERO) {
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
						this->upgrade[id].level += _ONE;
					}
				}

				this->upgrade[id].level = M;
				this->pre_alpha.score   = total * _to_Number(0.001);
			}

			// AU1-AU4.
			// Spend all available AUP.
			if (22 <= id && id < 26) {
				Number avail = this->_get_AUP_avaliable();
				if (avail >= _ONE) {
					this->upgrade[id].level = floor(this->upgrade[id].level + avail);
				}
			}

			// AUP Buy (id 26-29)
			// 26: matter       27: Amp. Aero
			// 28: Int. Aero    29: AP
			if (id >= 26 && id <= 29) {
				if (id == 26) {
					Number T = this->pre_alpha.score;
					if (T <= _ZERO) {
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
					Number S = T.log(_TEN) / _to_Number(100);

					Number M;
					if (S <= _to_Number(11)) {
						M = floor(S);
					} else {
						Number disc = S * _to_Number(4) - _to_Number(43);
						M = floor((disc.pow(_to_Number(0.5)) + _to_Number(21)) / _to_Number(2));
					}

					if (M <= x) {
						_try_upgrade(id);
						return;
					}

					if (M < x + _ONE) M = x + _ONE;

					this->upgrade[26].level = M;
					this->pre_alpha.score = T * _to_Number(0.001);
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

				if (s == _ONE) {
					// Degenerate case: linear cost
					Number n = floor(total / i);
					if (n < _ONE) return;
					this->upgrade[id].level += n;
					this->alpha.point      -= n * i;
					return;
				}

				// n = log_s( total * (s - 1) / (i * s^x) + 1 )
				Number inner = total * (s - _ONE) / (i * s.pow(x)) + _ONE;
				if (inner <= _ZERO) {
					_try_upgrade(id);
					return;
				}

				Number n = inner.log(s);
				n = floor(n);
				if (n < _ONE) {
					_try_upgrade(id);
					return;
				}

				// Exact cost: sum_{k=x}^{x+n-1} i * s^k
				Number cost = i * s.pow(x) * (s.pow(n) - _ONE) / (s - _ONE);
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
					this->upgrade[id].level = _ONE;
				}
			}

			// Alpha Amplifiers / Alpha Intensifiers (40-55)
			// Alpha Amp. / Int. Aeromagnifiers (56-57)
			if (40 <= id && id < 58) {
				Number x     = this->upgrade[id].level;
				Number total = this->alpha.score;
				if (total <= _ZERO) {
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
						this->upgrade[id].level += _ONE;
					}
				}

				this->upgrade[id].level = M;
				this->alpha.score   = total * _to_Number(0.001);
			}
			// ------------------------------------------------------------
			// Fallback: unknown id, do nothing.
			// ------------------------------------------------------------

			_try_upgrade(id);
			if (this->upgrade[20].level < _ZERO) this->upgrade[20].level = _ZERO;
			if (this->upgrade[21].level < _ZERO) this->upgrade[21].level = _ZERO;
		}
		// Upgrades all pre-prestige upgrades(or max all).
		// Includes A1-8,I1-8,AA1-AA8,AI1-AI8.
		void _try_upgrade_bulk_all() {
			Number cur = this->pre_alpha.score;
			for (int i = 0; i < 16; i ++) {
				this->pre_alpha.score = cur;
				_try_upgrade(i);
				_try_upgrade_bulk(i);
				if(cur < _to_Number(1e100)) cur = this->pre_alpha.score;
			}
			cur = this->alpha.score;
			if (!this->upgrade[34].level.is_zero()) {
				for (int i = 0; i < 16; i ++) {
					this->alpha.score = cur;
					_try_upgrade(i + 40);
					_try_upgrade_bulk(i + 40);
					if(cur < _to_Number(1e100)) cur = this->alpha.score;
				}
			}
		}

		[[nodiscard]] Number _get_game_speed() {
			Number base = _to_Number(_accel);
			return base;
		}
};

player_ player;
long double _get_time();

// Initialize the cost.
void cost_init() {
	for (int i = 0; i < 8; i ++) {
		player.upgrade[i].scale = _to_Number(1.5);
		player.upgrade[i].init_cost = _to_Number(1)
		                              * _TEN.pow(_to_Number(3 * i * i));
		player.upgrade[i + 8].scale = _to_Number(3);
		player.upgrade[i + 8].init_cost = _to_Number(100)
		                                  * _TEN.pow(_to_Number(3 * i * i));
		player.upgrade[40 + i].scale = _to_Number(1.5);
		player.upgrade[40 + i].init_cost = _to_Number(1)
		                                   * _TEN.pow(_to_Number(3 * i * i))
		                                   * _to_Number(1e2);
		player.upgrade[48 + i].scale = _to_Number(3);
		player.upgrade[48 + i].init_cost = _to_Number(100)
		                                   * _TEN.pow(_to_Number(3 * i * i))
		                                   * _to_Number(1e2);
		player.upgrade[58 + i].scale = _to_Number(1.5);
		player.upgrade[58 + i].init_cost = _to_Number(1)
		                                   * _TEN.pow(_to_Number(3 * i * i))
		                                   * _to_Number(1e3);
		player.upgrade[66 + i].scale = _to_Number(3);
		player.upgrade[66 + i].init_cost = _to_Number(100)
		                                   * _TEN.pow(_to_Number(3 * i * i))
		                                   * _to_Number(1e3);
	}
	player.upgrade[16].init_cost = _to_Number(1e16);
	player.upgrade[17].init_cost = _to_Number(1e30);
	player.upgrade[18].init_cost = _to_Number(1e50);
	player.upgrade[19].init_cost = _to_Number(1e100);
	player.upgrade[20].scale = _to_Number(1e3);
	player.upgrade[20].init_cost = _to_Number(1e10);
	player.upgrade[21].scale = _to_Number(1e5);
	player.upgrade[21].init_cost = _to_Number(1e20);

	player.upgrade[30].scale = _TEN;
	player.upgrade[30].init_cost = _TEN;
	player.upgrade[31].scale = _TEN;
	player.upgrade[31].init_cost = _to_Number(100);
	player.upgrade[32].scale = _to_Number(2);
	player.upgrade[32].init_cost = _to_Number(1e3);
	player.upgrade[33].scale = _to_Number(2);
	player.upgrade[33].init_cost = _to_Number(1e5);
	player.upgrade[34].scale = _ONE;
	player.upgrade[34].init_cost = _to_Number(1e6);
	player.upgrade[35].scale = _ONE;
	player.upgrade[35].init_cost = _to_Number(1e10);
	player.upgrade[36].init_cost = _to_Number(1e15);
	player.upgrade[36].scale = _ONE;
	player.upgrade[37].init_cost = _to_Number(1e20);
	player.upgrade[37].scale = _ONE;
	player.upgrade[38].init_cost = _to_Number(1e25);
	player.upgrade[38].scale = _ONE;
	player.upgrade[39].init_cost = _to_Number(1e30);
	player.upgrade[39].scale = _to_Number(100);

	player.upgrade[56].scale = _to_Number(1e3);
	player.upgrade[56].init_cost = _to_Number(1e10);
	player.upgrade[57].scale = _to_Number(1e5);
	player.upgrade[57].init_cost = _to_Number(1e20);
	player.upgrade[74].scale = _to_Number(1e3);
	player.upgrade[74].init_cost = _to_Number(1e10);
	player.upgrade[75].scale = _to_Number(1e5);
	player.upgrade[75].init_cost = _to_Number(1e20);
}


// Initialization.
void init() {
	player.pre_alpha.score = _ONE;				//Give 1 score to buy initial A1.
	/*
	player.options[0].val = _ZERO;				//Initial array presets.
	player.options[1].val = _to_Number(60);
	player.options[2].val = _to_Number(10);
	player.options[3].val_s = "save1.txt";
	player.options[4].val = _to_Number(0);
	player.options[5].val = _to_Number(50);
	*/
	cost_init();


	for (int i = 0; i < 1000; ++i) {
		player.ach[i] = false;
		player.story[i] = false;
	}

	player.GIM = _TEN;						//Initial GIM.
	player.PIM = _TEN;						//Initial PIM.
	player.PF = _TEN;						//Initial PF.
	player.pre_alpha.highest = _ONE;		//Initial score. (for prestige)
	player.last_tick = _get_time();			//Time init.
	player.currenttab = 1;					//Tab init.
	player.unlockedtabs = 3;
	player.unlocked_prestige = false;		//Self-explanatory.

	player.pre_alpha.time = _ZERO;
	player.alpha.time = _ZERO;
	player.fastest_ap_per_min = _ZERO;
	player.alpha.score   = _ONE;
	player.alpha.am_best = _ONE;
	player.alpha.am_rate = _ZERO;
	player.alpha.AIM     = _TEN;
	player.beta.score   = _ONE;
	player.beta.time = _ZERO;
}

// Initalization on an Aeromagnifier reset.
void init_aero() {
	player.pre_alpha.time = _ZERO;
	if(player._get_AM(0)) return;
	if(player.upgrade[19].level.is_one()) player.pre_alpha.score = _TEN * _TEN * _TEN * _TEN; //** PU4: Start with 10000.00 matter.
	else player.pre_alpha.score = _ONE;
	for (int i = 0; i < 8; i ++) {
		player.pre_alpha.amount[i] = _ONE;
		player.upgrade[i].level = _ZERO;
		player.upgrade[i + 8].level = _ZERO;
	}
}

// Initalization on an Alpha Aeromagnifier reset.
void init_alpha_aero() {
	player.alpha.score = _TEN * _TEN;
	for (int i = 0; i < 8; i ++) {
		player.alpha.amount[i] = _ONE;
		player.upgrade[i + 40].level = _ZERO;
		player.upgrade[i + 48].level = _ZERO;
	}
}

// Initialization after prestige once.
void init_prestige() {
	if(player.pre_alpha.score > player.pre_alpha.highest) player.pre_alpha.highest = player.pre_alpha.score; //Update highest score, for recalculate GIM, etc.
	init_aero();
	cost_init();
	if(player.upgrade[19].level.is_one()) player.pre_alpha.score = _TEN * _TEN * _TEN * _TEN; //** PU4: Start with 10000.00 matter.
	else player.pre_alpha.score = _ONE;
	for (int i = 0; i < 8; i ++) {
		player.pre_alpha.amount[i] = _ONE;
		player.upgrade[i].level = _ZERO;
		player.upgrade[i + 8].level = _ZERO;
	}
	player.upgrade[20].level = _ZERO;
	player.upgrade[21].level = _ZERO;
	player.pre_alpha.time = _ZERO;
	player.unlockedtabs = max(player.unlockedtabs, 4);
}

// Initialization after (performing) alpha once.
void init_alpha() {
	Number ap_gained = player._get_AP();
	player.alpha.point += ap_gained;
	player.alpha.alpha += player._get_alpha();

	// Update AP/min
	Number seconds = player.alpha.time;
	if (seconds > _ZERO) {
		Number ap_per_min = ap_gained * _to_Number(60) / seconds;
		if (ap_per_min > player.fastest_ap_per_min) {
			player.fastest_ap_per_min = ap_per_min;
		}
	}

	init_prestige();
	player.pre_alpha.score = _ONE;
	for(int i = 16; i < 20; i ++) {
		if(!player._get_AM(4)) player.upgrade[i].level = _ZERO;
	}
	for(int i = 20; i < 22; i ++) {
		player.upgrade[i].level = _ZERO;
	}

	player.last_tick = _get_time();
	player.unlockedtabs = max(player.unlockedtabs, 5);
	player.unlocked_alpha = true;
	player.alpha.time = _ZERO;
	player.pre_alpha.time = _ZERO;
	player.pre_alpha.highest = _ONE;
}

// Initialization after RESPECCING alpha once. (AUP respec, etc.)
// Notice that respeccing isn't equal to performing an alpha, or any respective prestige layer.
// While performing an alpha gives you reward, this doesn't give you any reward from your current alpha.
// It seems that respeccing is a waste of time, but it will be widely used for various strategies.
void respec_alpha() {
	// player.alpha.point += player._get_AP();
	// player.alpha.alpha += player._get_alpha();
	init_prestige();
	player.pre_alpha.score = _ONE;
	for(int i = 16; i < 20; i ++) {
		if(!player._get_AM(4)) player.upgrade[i].level = _ZERO;
	}
	for(int i = 20; i < 22; i ++) {
		player.upgrade[i].level = _ZERO;
	}

	player.last_tick = _get_time();
	player.unlockedtabs = max(player.unlockedtabs, 5);
	player.unlocked_alpha = true;
	player.alpha.time = _ZERO;
	player.pre_alpha.time = _ZERO;
	player.pre_alpha.highest = _ONE;
}

void init_beta() {
	Number bp_gained = player._get_BP();
	player.beta.point += bp_gained;
	player.beta.beta += player._get_beta();

	// Update BP/min
	Number seconds = player.beta.time;
	if (seconds > _ZERO) {
		Number bp_per_min = bp_gained * _to_Number(60) / seconds;
		if (bp_per_min > player.fastest_bp_per_min) {
			player.fastest_bp_per_min = bp_per_min;
		}
	}

	init_alpha();
	player.alpha.score = _ONE;
	for(int i = 0; i < 58; i ++) {
		player.upgrade[i].level = _ZERO;
	}
	for(int i = 0; i < 8; i ++){
		player.alpha.amount[i] = _ONE;
	}
	player.upgrade[34].level = _ONE;
	player.upgrade[35].level = _ONE;
	for(int i = 1; i <= 48; i ++) {
		player.ach[i] = false;
	}

	player.last_tick = _get_time();
	player.unlockedtabs = max(player.unlockedtabs, 6);
	player.unlocked_alpha = true;
	player.break_alpha = false;
	player.unlocked_beta = true;
	player.alpha.highest = _ONE;
	player.alpha.am_best = _ONE;
	player.alpha.am_rate = _ONE;
	player.alpha.alpha = _ZERO;
	player.alpha.point = _ZERO;
	player.beta.highest = _ONE;
	player.beta.highest_am = _ONE;

	player.beta.time = _ZERO;
	player.fastest_ap_per_min = _ZERO;
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

static ImVec4 _get_BD_color(){
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

	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Project: Simulation v1.0.0: The Augmentation Apeirogon Update.",
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

static std::string N2S_save(const Number& n) {
	Number a = n.normalize();
	std::ostringstream oss;
	oss << "N("
	    << std::fixed << std::setprecision(10) << a.val   << ","
	    << a.layer                                        << ","
	    << (a.sign ? 1 : 0)                               << ","
	    << a.neg                                          << ")";
	return oss.str();
}

//Init. draw tabs.
void DrawMatterTab();
void DrawOptionsTab();
void DrawPrestigeTab();
void DrawTopBar();
void DrawAlphaTab();
void DrawStoryTab();

// A1-A8 / I1-I8 Buy mode switch
// true  = Buy Max
// false = Buy 1
static bool g_bulk_buy = true;

// Static holder for calculation results.
static Number m_rate[8] = {_ZERO,_ZERO,_ZERO,_ZERO,_ZERO,_ZERO,_ZERO,_ZERO};

//An upgrade card includes A(i+1) and I(i+1).
void DrawUpgradeCard(int i) {
	if (i % 4 != 0) ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::BeginChild(("##upg" + to_string(i)).c_str(), ImVec2(300, 200), true);

	ImGui::Text("M%d = %s", i + 1, N2S(player.pre_alpha.amount[i]).c_str());
	ImGui::Separator();

	// Amplifier
	Number a = _to_Number(3)
	           * player.upgrade[i].level
	           * player._get_aero_effect_a()
	           * player._get_ach_effect(1)
	           * player._get_BAU_effect(1)
	           * player._get_Am_effect(3);
	if(player.upgrade[17].level.is_one()) a = a * player._get_PU2_effect();
	if(player._get_ach(32)) a = a * player._get_PU2_effect(player.pre_alpha.highest);
	if(player._get_story(1)) a = a * player._get_story_effect(1);
	Number lev = player.upgrade[i + 8].level + _ONE;
	Number exp = lev.pow_sum(player._get_GIM_effect() - 1)
	             * player._get_aero_effect_i()
	             * player._get_Am_effect(4);

	//Softcap
	if(exp >= _to_Number(5e6)) exp = exp.pow(_to_Number(0.5)) * _to_Number(5e6).pow(_to_Number(0.5));
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

void DrawAlphaUpgradeCard(int i) {
	if (i % 4 != 0) ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::BeginChild(("##aupg" + to_string(i)).c_str(), ImVec2(300, 200), true);

	ImGui::Text("AM%d = %s", i + 1, N2S(player.alpha.amount[i]).c_str());
	ImGui::Separator();

	// Alpha Amplifier
	Number aa_base = _to_Number(3)
	                 * player.upgrade[40 + i].level
	                 * player._get_alpha_aero_effect_a()
	                 * player._get_story_effect(3);

	if (player.upgrade[36].level.is_one()) {
		aa_base = aa_base * player._get_ach_effect(1);
	}
	if (player._get_ach(38)) {
		aa_base = aa_base * player._get_story_effect(1);
	}
	if (player._get_ach(48)) {
		Number base = player.pre_alpha.score;
		Number mult = _ONE;
		if(base >= _TEN) {
			Number base_log = base.log(_TEN);
			if(base_log >= _TEN) {
				mult = base_log.log(_TEN);
			}
		}
		aa_base = aa_base * mult;
	}

	Number lev = player.upgrade[48 + i].level + _ONE;
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

void DrawBetaUpgradeCard(int i) {
	if (i % 4 != 0) ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::BeginChild(("##bupg" + to_string(i)).c_str(), ImVec2(300, 200), true);

	ImGui::Text("BM%d = %s", i + 1, N2S(player.beta.amount[i]).c_str());
	ImGui::Separator();

	// Beta Amplifier
	Number ba_base = _to_Number(3)
	                 * player.upgrade[58 + i].level
	                 * player._get_story_effect(3);

	if (player._get_ach(38)) {
		ba_base = ba_base * player._get_story_effect(1);
	}
	if (player._get_ach(48)) {
		Number base = player.pre_alpha.score;
		Number mult = _ONE;
		if(base >= _TEN) {
			Number base_log = base.log(_TEN);
			if(base_log >= _TEN) {
				mult = base_log.log(_TEN);
			}
		}
		ba_base = ba_base * mult;
	}

	Number lev = player.upgrade[66 + i].level + _ONE;
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
void DrawMatterTab() {
	ImGui::SetNextWindowPos(ImVec2(0, 42));
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - 42));
	ImGui::Begin("##Matter", nullptr,
	             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
	             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

	ImGui::Text("Matter: %s  ( +%s/s", N2S(player.pre_alpha.score).c_str(), N2S(rate).c_str());
	if(player.unlocked_beta) {
		string positive = "";
		if(player.rate_real > _ZERO) positive = "+";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),"->%s%s/s", positive.c_str(), N2S(player.rate_real).c_str());
	}
	ImGui::SameLine();
	ImGui::Text(")");

	if(player.unlocked_beta) {
		string s = "x";
		if(player.pre_alpha.score >= _TEN.pow(_TEN)) s = "^";
		if(player.pre_alpha.score >= _TEN.pow(_TEN.pow(_TEN))) s = "^^";
		if(player.pre_alpha.score >= _TEN.pow(_TEN.pow(_TEN.pow(_TEN)))) s = "^^^";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),
		                   "Your Matters are decaying. Matter: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.pre_alpha.score, _ONE)).c_str());
		if(player.upgrade[34].level.is_one()) {
			s = "x";
			if(player.alpha.score >= _TEN.pow(_TEN)) s = "^";
			if(player.alpha.score >= _TEN.pow(_TEN.pow(_TEN))) s = "^^";
			if(player.alpha.score >= _TEN.pow(_TEN.pow(_TEN.pow(_TEN)))) s = "^^^";
			ImGui::SameLine();
			ImGui::TextColored(_get_BD_color(),
			                   " Am: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.alpha.score, _ONE)).c_str());
		}
		s = "x";
		if(player.beta.score >= _TEN.pow(_TEN)) s = "^";
		if(player.beta.score >= _TEN.pow(_TEN.pow(_TEN))) s = "^^";
		if(player.beta.score >= _TEN.pow(_TEN.pow(_TEN.pow(_TEN)))) s = "^^^";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),
		                   " Bm: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.beta.score, _ONE)).c_str());
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
		ImGui::Text("Matter: %s  ( +%s/s",
			            N2S(player.pre_alpha.score).c_str(),
			            N2S(rate).c_str());
			ImGui::SameLine();
			if(player.unlocked_beta){
				ImGui::TextColored(_get_BD_color(), "->%s/s", N2S(player.rate_real).c_str());
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
			DrawUpgradeCard(i);
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
			ImGui::Text("Alpha Matter (Am): %s  ( +%s/s",
			            N2S(player.alpha.score).c_str(),
			            N2S(player.alpha.am_rate).c_str());
			ImGui::SameLine();
			if(player.unlocked_beta){
				ImGui::TextColored(_get_BD_color(), "->%s/s", N2S(player.rate_real_Am).c_str());
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
				DrawAlphaUpgradeCard(i);
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
		ImGui::Text("Beta Matter (Bm): %s  ( +%s/s",
		            N2S(player.beta.score).c_str(),
		            N2S(player.beta.bm_rate).c_str());
		ImGui::SameLine();
		if(player.unlocked_beta){
			ImGui::TextColored(_get_BD_color(), "->%s/s", N2S(player.rate_real_Bm).c_str());
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
			DrawBetaUpgradeCard(i);
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
			if(ImGui::Button(("Prestige"))) init_prestige();
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
			if(ImGui::Button(("Prestige"))) init_prestige();
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
			if(ImGui::Button(("Alpha"))) init_alpha();
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
			if(ImGui::Button(("Beta"))) init_beta();
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
				if(ok && !player._get_AM(0)) init_aero();
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
				if(ok) init_alpha_aero();
			}
		}
	}

	ImGui::End();
}

static std::string get_timestamp();
static std::string get_save_string();
static void load_game_state_from_string(const std::string& data);
void save_game_state();
void load_game_state(const string& filename);
static std::string paste_from_clipboard();
#ifdef _WIN32
static void copy_to_clipboard(const std::string& text);
#endif

int optionstab = 1;
//Options tab.
void DrawOptionsTab() {
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
			player.options[1].val = _to_Number((long double)sec_amount);

		ImGui::RadioButton("Percent", &flux_mode, 1);
		ImGui::SameLine();
		static float pct_amount = 10.0f;
		pct_amount = (float)player.options[2].val._to_long_double();
		if (ImGui::DragFloat("##pct", &pct_amount, 1.0f, 0.0f, 100.0f, "%.1f %%"))
			player.options[2].val = _to_Number((long double)pct_amount);
		player.options[0].val = _to_Number(flux_mode);

		if (ImGui::Button("Activate (Shift+Enter)", ImVec2(240, 0))) {
			long double amt = (flux_mode == 0) ? sec_amount
			                  : player.time_flux * pct_amount / 100.0L;
			if (amt > player.time_flux) amt = player.time_flux;
			if (amt > 0) {
				player.time_flux -= amt;
				flux_pool = _to_Number(amt);
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
			player.options[4].val = _to_Number(song_index);

		static float volume = 50.0f;
		volume = (float)player.options[5].val._to_long_double();
		if (ImGui::SliderFloat("Volume", &volume, 0.0f, 100.0f, "%.0f%%"))
			player.options[5].val = _to_Number((long double)volume);

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
				load_game_state(target);
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
void DrawPrestigeTab() {
	ImGui::SetNextWindowPos(ImVec2(0, 42));
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - 42));
	ImGui::Begin("##Prestige", nullptr,
	             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
	             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

	ImGui::Text("Matter: %s  ( +%s/s", N2S(player.pre_alpha.score).c_str(), N2S(rate).c_str());
	if(player.unlocked_beta) {
		string positive = "";
		if(player.rate_real > _ZERO) positive = "+";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),"->%s%s/s", positive.c_str(), N2S(player.rate_real).c_str());
	}
	ImGui::SameLine();
	ImGui::Text(")");

	if(player.unlocked_beta) {
		string s = "x";
		if(player.pre_alpha.score >= _TEN.pow(_TEN)) s = "^";
		if(player.pre_alpha.score >= _TEN.pow(_TEN.pow(_TEN))) s = "^^";
		if(player.pre_alpha.score >= _TEN.pow(_TEN.pow(_TEN.pow(_TEN)))) s = "^^^";
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.0f, 1.0f),
		                   "Your Matters are decaying. Matter: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.pre_alpha.score, _ONE)).c_str());
		if(player.upgrade[34].level.is_one()) {
			s = "x";
			if(player.alpha.score >= _TEN.pow(_TEN)) s = "^";
			if(player.alpha.score >= _TEN.pow(_TEN.pow(_TEN))) s = "^^";
			if(player.alpha.score >= _TEN.pow(_TEN.pow(_TEN.pow(_TEN)))) s = "^^^";
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.0f, 1.0f),
			                   " Am: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.alpha.score, _ONE)).c_str());
		}
	}

	ImGui::Text("GIM = 10-logPF(log10(Prest.)) = %s -> ^%s to Mult, PIM: %s -> PF = %s.",
	            N2S(player._get_GIM()).c_str(), N2S(player._get_GIM_effect()).c_str(),
	            N2S(player._get_PIM()).c_str(), N2S(player._get_PF()).c_str());

	ImGui::Separator();

	const char* pu_names[4] = {
		"(PU1) Unlock Aeromagnifiers",
		"(PU2) Give a mult. to M/s before Int. effect",
		"(PU3) Reduce PIM by 10% GIM reduction",
		"(PU4) Start with 10000 matter"
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
			if(ImGui::Button(("Prestige"))) init_prestige();
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
			if(ImGui::Button(("Prestige"))) init_prestige();
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
			if(ImGui::Button(("Alpha"))) init_alpha();
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
			if(ImGui::Button(("Beta"))) init_beta();
		}
	}


	ImGui::End();
}


// Alpha tab.
void DrawAlphaTab() {
	ImGui::SetNextWindowPos(ImVec2(0, 42));
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - 42));
	ImGui::Begin("##Alpha", nullptr,
	             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
	             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

	ImGui::Text("Matter: %s  ( +%s/s", N2S(player.pre_alpha.score).c_str(), N2S(rate).c_str());
	if(player.unlocked_beta) {
		string positive = "";
		if(player.rate_real > _ZERO) positive = "+";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),"->%s%s/s", positive.c_str(), N2S(player.rate_real).c_str());
	}
	ImGui::SameLine();
	ImGui::Text(")");

	if(player.unlocked_beta) {
		string s = "x";
		if(player.pre_alpha.score >= _TEN.pow(_TEN)) s = "^";
		if(player.pre_alpha.score >= _TEN.pow(_TEN.pow(_TEN))) s = "^^";
		if(player.pre_alpha.score >= _TEN.pow(_TEN.pow(_TEN.pow(_TEN)))) s = "^^^";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),
		                   "Your Matters are decaying. Matter: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.pre_alpha.score, _ONE)).c_str());
		if(player.upgrade[34].level.is_one()) {
			s = "x";
			if(player.alpha.score >= _TEN.pow(_TEN)) s = "^";
			if(player.alpha.score >= _TEN.pow(_TEN.pow(_TEN))) s = "^^";
			if(player.alpha.score >= _TEN.pow(_TEN.pow(_TEN.pow(_TEN)))) s = "^^^";
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.0f, 1.0f),
			                   " Am: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.alpha.score, _ONE)).c_str());
		}
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
	if(AlphaButton(("Alpha Milestone"))) player.alpha.tab = 3;

	// Alpha Upgrades tab.
	if(player.alpha.tab == 1) {
		ImGui::Text("You have %s AP and %s Alpha.",
		            N2S(player.alpha.point).c_str(),
		            N2S(player.alpha.alpha).c_str());
		ImGui::Text("Respec AUP Upgrades(which also performs an Alpha reset) to redistribute them.");
		ImGui::SameLine();
		if(AlphaButton("Respec")) {
			respec_alpha();
			for(int i = 22; i < 26; i ++) player.upgrade[i].level = _ZERO;
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
				if(ImGui::Button(("Prestige"))) init_prestige();
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
				if(ImGui::Button(("Prestige"))) init_prestige();
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
				if(ImGui::Button(("Alpha"))) init_alpha();
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
				Number mul = _ONE;
				Number s = player.pre_alpha.score;
				if (s > _ONE) {
					Number log_s = s.log(_TEN);
					if (log_s > _ZERO) {
						Number x = player.upgrade[32].level;
						Number exponent = _to_Number(0.75) - _to_Number(0.75) / _to_Number(1.1).pow(x);
						if (exponent > _to_Number(0.75)) exponent = _to_Number(0.75);
						mul = log_s.pow(exponent);
					}
				}
				effect = "Currently: x" + N2S(mul);
			} else if (i == 3) {
				// BAU4
				Number rate = _ZERO;
				if (player.upgrade[33].level > _ZERO && player.fastest_ap_per_min > _ZERO) {
					rate = player.fastest_ap_per_min * player.upgrade[33].level * _to_Number(0.01);
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
	// Alpha Milestone tab.
	if(player.alpha.tab == 3) {
		ImGui::Text("You have %s AP and %s Alpha.",
		            N2S(player.alpha.point).c_str(),
		            N2S(player.alpha.alpha).c_str());
		ImGui::BeginChild("##am", ImVec2(-1, 700), true);
		ImGui::Text("You have %s Alpha.",
		            N2S(player.alpha.alpha).c_str());
		for(int i = 0; i < 7; i ++) {
			ImGui::BeginChild(("##am" + to_string(i)).c_str(), ImVec2(-1, 80), true);
			if(player.alpha.alpha < am[i]) {
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
		ImGui::EndChild();
	}
	ImGui::End();
}

// Beta tab.
void DrawBetaTab() {
	ImGui::SetNextWindowPos(ImVec2(0, 42));
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - 42));
	ImGui::Begin("##Beta", nullptr,
	             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
	             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

	ImGui::Text("Matter: %s  ( +%s/s", N2S(player.pre_alpha.score).c_str(), N2S(rate).c_str());
	if(player.unlocked_beta) {
		string positive = "";
		if(player.rate_real > _ZERO) positive = "+";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),"->%s%s/s", positive.c_str(), N2S(player.rate_real).c_str());
	}
	ImGui::SameLine();
	ImGui::Text(")");

	if(player.unlocked_beta) {
		string s = "x";
		if(player.pre_alpha.score >= _TEN.pow(_TEN)) s = "^";
		if(player.pre_alpha.score >= _TEN.pow(_TEN.pow(_TEN))) s = "^^";
		if(player.pre_alpha.score >= _TEN.pow(_TEN.pow(_TEN.pow(_TEN)))) s = "^^^";
		ImGui::SameLine();
		ImGui::TextColored(_get_BD_color(),
		                   "Your Matters are decaying. Matter: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.pre_alpha.score, _ONE)).c_str());
		if(player.upgrade[34].level.is_one()) {
			s = "x";
			if(player.alpha.score >= _TEN.pow(_TEN)) s = "^";
			if(player.alpha.score >= _TEN.pow(_TEN.pow(_TEN))) s = "^^";
			if(player.alpha.score >= _TEN.pow(_TEN.pow(_TEN.pow(_TEN)))) s = "^^^";
			ImGui::SameLine();
			ImGui::TextColored(_get_BD_color(),
			                   " Am: %s%s/s.", s.c_str(), N2S(player._get_BD_effect(player.alpha.score, _ONE)).c_str());
		}
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
  		ImGui::TextColored(dark_red, "Step 1: If a <= 1e10, x = log10(a), a is multiplied by k per second.");
  		ImGui::TextColored(dark_red, "Step 2: If 1e10 < a <= e10000000000, x = log10(log10(a)), a is empowered by k per second.");
  		ImGui::TextColored(dark_red, "Step 3: If e10000000000 < a <= e1e10000000000, x = triple-log10(a), a is dilated by k per second.");
  		ImGui::TextColored(dark_red, "Step 4: If e1e10000000000 < a, x = quadrable-log10(a), a is tetrated by k per second.\n");
  		
		ImGui::Text("");
  		string s;
  		if(player._get_BD_step(player.pre_alpha.score) == _ONE) s = "1. Matter/s is being multiplied by ";
  		if(player._get_BD_step(player.pre_alpha.score) == _to_Number(2)) s = "2. Matter/s is being empowered by ";
  		if(player._get_BD_step(player.pre_alpha.score) == _to_Number(3)) s = "3. Matter/s is being dilated by ";
  		if(player._get_BD_step(player.pre_alpha.score) == _to_Number(4)) s = "4. Matter/s is being tetrated by ";
  		ImGui::TextColored(dark_red, "You have %s matter, which is in Step %s%s. Current Matter is affected by the same way per second.",
			 						N2S(player.pre_alpha.score).c_str(),
									s.c_str(),
									N2S(player._get_BD_effect(player.pre_alpha.score, _ONE)).c_str());
		
		if(player._get_BD_step(player.alpha.score) == _ONE) s = "1. Alpha Matter/s is being multiplied by ";
  		if(player._get_BD_step(player.alpha.score) == _to_Number(2)) s = "2. Alpha Matter/s is being empowered by ";
  		if(player._get_BD_step(player.alpha.score) == _to_Number(3)) s = "3. Alpha Matter/s is being dilated by ";
  		if(player._get_BD_step(player.alpha.score) == _to_Number(4)) s = "4. Alpha Matter/s is being tetrated by ";
  		ImGui::TextColored(dark_red, "You have %s Alpha matter, which is in Step %s%s. Current Alpha Matter is affected by the same way per second.",
			 						N2S(player.alpha.score).c_str(),
									s.c_str(),
									N2S(player._get_BD_effect(player.alpha.score, _ONE)).c_str());
		
		if(player._get_BD_step(player.beta.score) == _ONE) s = "1. Beta Matter/s is being multiplied by ";
  		if(player._get_BD_step(player.beta.score) == _to_Number(2)) s = "2. Beta Matter/s is being empowered by ";
  		if(player._get_BD_step(player.beta.score) == _to_Number(3)) s = "3. Beta Matter/s is being dilated by ";
  		if(player._get_BD_step(player.beta.score) == _to_Number(4)) s = "4. Beta Matter/s is being tetrated by ";
  		ImGui::TextColored(dark_red, "You have %s Beta matter, which is in Step %s%s. Current Beta Matter is affected by the same way per second.",
			 						N2S(player.beta.score).c_str(),
									s.c_str(),
									N2S(player._get_BD_effect(player.beta.score, _ONE)).c_str());
		
		
	}
	// Beta Upgrades tab.
	if(player.beta.tab == 2) {
		ImGui::Text("You have %s BP and %s Beta.",
		            N2S(player.beta.point).c_str(),
		            N2S(player.beta.beta).c_str());
	}
	// Beta Challenges tab.
	if(player.beta.tab == 3) {
	}
	// Beta Cosmology Tree tab.
	if(player.beta.tab == 4) {
	}
	// Beta Milestone tab.
	if(player.beta.tab == 5) {
	}
	ImGui::End();
}

int storytab = 1;

// Achievement tab.
void DrawStoryTab() {
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
		if(player._get_ach(44)) ImGui::Text("Alpha Intensifiers: x%s", N2S(player._get_ach_effect(1)).c_str());
		for(int i = 1; i <= 48; i ++) {
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
		for(int i = 1; i <= 5; i ++) {
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

void DrawTopBar() {
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

void GUI_RenderFrame() {
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
		DrawTopBar();

		switch (player.currenttab) {
			case 1:
				DrawMatterTab();
				break;
			case 2:
				DrawOptionsTab();
				break;
			case 3:
				DrawStoryTab();
				break;
			case 4:
				DrawPrestigeTab();
				break;
			case 5:
				DrawAlphaTab();
				break;
			case 6:
				DrawBetaTab();
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
void update(Number s) {
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
			player.options[0].val = _to_Number(sel);
			editing_option = false;
		}
		if (downNow && !prevDown) {
			sel = (sel + 1) % 5;
			player.options[0].val = _to_Number(sel);
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
				player.options[1].val = _to_Number(v);
			} else if (sel == 1) {
				long double v = player.options[2].val._to_long_double();
				if (leftNow  && !prevLeftEdit)  v -= 1.0L;
				if (rightNow && !prevRightEdit) v += 1.0L;
				if (v < 0)   v = 0;
				if (v > 100) v = 100;
				player.options[2].val = _to_Number(v);
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
				player.options[4].val = _to_Number(v);
			} else if(sel == 4) { //BGM Volume
				long double v = player.options[5].val._to_long_double();
				if (leftNow  && !prevLeftEdit)  v -= 1.0L;
				if (rightNow && !prevRightEdit) v += 1.0L;
				if (v < 0)   v = 0;
				if (v > 100) v = 100;
				player.options[5].val = _to_Number(v);
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
				flux_pool = _to_Number(pool_amount);
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
			init_prestige();
		}
	}
	if (aNow && !prevA && capsOn) {
		if (player.pre_alpha.score >= inf) {
			init_alpha();
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
				if(cur >= _to_Number(1e100)) player.pre_alpha.score = cur;
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
				if(cur >= _to_Number(1e100)) player.pre_alpha.score = cur;
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
			if(cur >= _to_Number(1e100)) player.pre_alpha.score = cur;
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
			if(cur >= _to_Number(1e100)) player.pre_alpha.score = cur;
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
		if(cur >= _to_Number(1e100)) player.pre_alpha.score = cur;
		Number x = player.upgrade[20].level;
		player._try_upgrade(20);
		while(x != player.upgrade[20].level) {
			x = player.upgrade[20].level;
			player._try_upgrade(20);
		}
		if(ok && !player._get_AM(0)) init_aero();
	}
	if (((!block_hotkeys && shiftNow && isKeyDown('I'))
	        || (player.upgrade[21].autobuyer.enabled && player._get_AM(5)))
	        && player.upgrade[16].level.is_one()) {
		bool ok = (player._level_cost(21, player.upgrade[21].level) <= player.pre_alpha.score);
		Number cur = player.pre_alpha.score;
		player._try_upgrade_bulk(21);
		if(cur >= _to_Number(1e100)) player.pre_alpha.score = cur;
		Number x = player.upgrade[21].level;
		player._try_upgrade(21);
		while(x != player.upgrade[21].level) {
			x = player.upgrade[21].level;
			player._try_upgrade(21);
		}
		if(ok && !player._get_AM(0)) init_aero();
	}

	//Time and income calculation.
	Number tem = _time_flux_accel(s);
	Number effective_s = player._get_game_speed() * s;

	// Safety clamp for effective_s
	if (effective_s.is_negative() || !std::isfinite(effective_s._to_long_double())) {
		effective_s = _ZERO;
	}
	player.pre_alpha.time += effective_s;
	player.alpha.time += effective_s;

	// ** BAU4: Passive AP generation.
	if (player.upgrade[33].level > _ZERO && player.fastest_ap_per_min > _ZERO) {
		Number ap_per_min = player.fastest_ap_per_min * player.upgrade[33].level * _to_Number(0.01);
		Number ap_per_sec = ap_per_min / _to_Number(60.0);
		player.alpha.point += ap_per_sec * effective_s;
		player.alpha.point = player.alpha.point.normalize();
	}

	// ============================================================
	// Amplifier / Intensifier Production & Matter Accumulation.
	// ============================================================
	Number add = _ONE;
	for (int i = 0; i < 8; i ++) {

		// Base effect calculation
		Number a = _to_Number(3)
		           * player.upgrade[i].level.normalize()
		           * player._get_aero_effect_a()
		           * player._get_ach_effect(1)
		           * player._get_BAU_effect(1)
		           * player._get_Am_effect(3);

		if (player.upgrade[17].level.is_one()) a = a * player._get_PU2_effect();
		if (player._get_ach(32)) a = a * player._get_PU2_effect(player.pre_alpha.highest);
		if (player._get_story(1)) a = a * player._get_story_effect(1);

		// CRITICAL FIX: Force 'a' to be non-negative after all multiplications
		if (a.is_negative() || a.is_zero()) {
			a = _ZERO;
		}

		// Intensifier exponent
		Number lev = (player.upgrade[i + 8].level + _ONE);
		Number exp_val = player._get_GIM_effect() - _ONE; // Usually -0.5 initially

		// Calculate exponent multiplier from intensifiers/aeros
		Number exp_mult = lev.pow_sum(exp_val)
		                  * player._get_aero_effect_i()
		                  * player._get_Am_effect(4);

		// Softcap for exponent
		if(exp_mult >= _to_Number(5e6)) exp_mult = exp_mult.pow(_to_Number(0.5)) * _to_Number(5e6).pow(_to_Number(0.5));

		//cout << a << " " << exp_mult << " ";
		//cout << effective_s << " " << a.pow(exp_mult) << " " << a.pow(exp_mult) * effective_s << " " << player.pre_alpha.amount[i] << endl;
		//if(i == 0) cout << player.pre_alpha.amount[i] << "+" << m_rate[i] << "*" << effective_s << "(=" << m_rate[i] * effective_s << ")=" << player.pre_alpha.amount[i] + m_rate[i] * effective_s << endl << endl;
		player.pre_alpha.amount[i] = player.pre_alpha.amount[i] + a.pow(exp_mult) * effective_s;

		// CRITICAL FIX: Clamp amount to non-negative
		if (player.pre_alpha.amount[i].is_negative()) {
			player.pre_alpha.amount[i] = _ZERO;
		}

		add = add * player.pre_alpha.amount[i];
	}

	rate = add.pow(player._get_GIM_effect());
	rate = rate.normalize();

	// CRITICAL FIX: Ensure rate is non-negative
	if (rate.is_negative()) {
		rate = _ZERO;
	}
	Number score_delta = rate * effective_s;
	score_delta = player._get_BD(score_delta, _ONE);
	
	Number score = player.pre_alpha.score;
	player.pre_alpha.score = player.pre_alpha.score + score_delta;
	player.pre_alpha.score = player.pre_alpha.score.normalize();
	
	player.pre_alpha.score = player._get_BD(player.pre_alpha.score, effective_s);
	player.rate_real = (player.pre_alpha.score - score) / effective_s;

	// CRITICAL FIX: Prevent score from dropping below zero or becoming NaN
	if (player.pre_alpha.score.is_negative()) {
		player.pre_alpha.score = _ONE; // Reset to 1 to allow buying again
		player.pre_alpha.score.neg = 1;
	}
	if (!player.upgrade[2].level.is_zero()) player.unlocked_prestige = true;

	// ============================================================
	// Alpha Amplifier / Alpha Intensifier Production & Am Accumulation.
	// ============================================================
	if (player.upgrade[34].level.is_one()) {
		Number am_add = _ONE;
		for (int i = 0; i < 8; i++) {
			// AA base: 3 * AA level * Alpha Amp. Aero effect.
			Number aa = _to_Number(3)
			            * player.upgrade[40 + i].level
			            * player._get_alpha_aero_effect_a()
			            * player._get_story_effect(3);

			if (player.upgrade[36].level.is_one()) {
				aa = aa * player._get_ach_effect(1);
			}
			if (player._get_ach(38)) {
				aa = aa * player._get_story_effect(1);
			}
			if (player._get_ach(48)) {
				Number base = player.pre_alpha.score;
				Number mult = _ONE;
				if(base >= _TEN) {
					Number base_log = base.log(_TEN);
					if(base_log >= _TEN) {
						mult = base_log.log(_TEN);
					}
				}
				aa = aa * mult;
			}

			// AI exponent: uses AIM_effect and Alpha Int. Aero effect.
			Number lev_ai = player.upgrade[48 + i].level + _ONE;
			Number exp_ai = lev_ai.pow_sum(player._get_AIM_effect() - 1)
			                * player._get_alpha_aero_effect_i();

			Number effect_ai = aa.pow(exp_ai);

			Number am_delta = effective_s * effect_ai;

			player.alpha.amount[i] = player.alpha.amount[i] + am_delta;
			player.alpha.amount[i] = player.alpha.amount[i].normalize();


			if (player.alpha.amount[i].is_negative()) {
				player.alpha.amount[i] = _ONE;
			}

			am_add = am_add * player.alpha.amount[i];

			if (std::isnan(am_add._to_long_double())) am_add = _ONE;
		}

		Number am_exp = player._get_AIM_effect();
		player.alpha.am_rate = am_add.pow(am_exp).normalize();


		if (player._get_story(4)) {
			player.alpha.am_rate = player.alpha.am_rate * player._get_story_effect(4);
		}
		player.alpha.am_rate = player.alpha.am_rate * player._get_BAU8_effect();
		if (player._get_ach(39)) {
			Number base = player.alpha.am_best;
			if (base > _ONE) {
				Number log10_am = base.log(_TEN);
				if (log10_am > _ONE) {
					player.alpha.am_rate = player.alpha.am_rate * log10_am;
				}
			}
		}
		if (player._get_ach(40)) {
			Number base = player.alpha.point;
			if (base > _ONE) {
				Number log10_ap = base.log(_TEN);
				if (log10_ap > _ONE) {
					player.alpha.am_rate = player.alpha.am_rate * log10_ap;
				}
			}
		}
		Number am_score_delta = player.alpha.am_rate * effective_s;
		am_score_delta = player._get_BD(am_score_delta, _ONE);
		
		Number alpha_score = player.alpha.score;
		player.alpha.score = player.alpha.score + am_score_delta;
		player.alpha.score = player.alpha.score.normalize();
		
		player.alpha.score = player._get_BD(player.alpha.score, effective_s);

		player.rate_real_Am = (player.alpha.score - alpha_score) / effective_s;

		if (player.alpha.score.is_negative()) {
			player.alpha.score = _ONE;
		}

		if(player.beta.highest_am < player.alpha.score) player.beta.highest_am = player.alpha.score;
	} else {
		// BAU5 not bought: no Am production, no Am effect.
		player.alpha.am_rate = _ZERO;
		for (int i = 0; i < 8; i++) player.alpha.amount[i] = _ONE;
	}
	
	// ============================================================
	// Beta Amplifier / Beta Intensifier Production & Bm Accumulation.
	// ============================================================
	if (player.unlocked_beta) {
		Number bm_add = _ONE;
		for (int i = 0; i < 8; i ++) {
			Number ba = _to_Number(3)
			            * player.upgrade[58 + i].level
			            * player._get_beta_aero_effect_a()
			            * player._get_story_effect(3);

			Number lev_bi = player.upgrade[66 + i].level + _ONE;
			Number exp_bi = lev_bi.pow_sum(player._get_BIM_effect() - 1)
			                * player._get_beta_aero_effect_i();

			Number effect_bi = ba.pow(exp_bi);

			Number bm_delta = effective_s * effect_bi;

			player.beta.amount[i] = player.beta.amount[i] + bm_delta;
			player.beta.amount[i] = player.beta.amount[i].normalize();


			if (player.beta.amount[i].is_negative()) {
				player.beta.amount[i] = _ONE;
			}

			bm_add = bm_add * player.alpha.amount[i];

			if (std::isnan(bm_add._to_long_double())) bm_add = _ONE;
		}

		Number bm_exp = player._get_BIM_effect();
		player.beta.bm_rate = bm_add.pow(bm_exp).normalize();

		Number bm_score_delta = player.beta.bm_rate * effective_s;
		bm_score_delta = player._get_BD(bm_score_delta, _ONE);
		
		Number beta_score = player.beta.score;
		player.beta.score = player.beta.score + bm_score_delta;
		player.beta.score = player.beta.score.normalize();
		
		player.beta.score = player._get_BD(player.beta.score, effective_s);

		player.rate_real_Bm = (player.beta.score - beta_score) / effective_s;

		if (player.beta.score.is_negative()) {
			player.beta.score = _ONE;
		}

		if(player.beta.highest_bm < player.beta.score) player.beta.highest_bm = player.beta.score;
	} else {
		player.beta.bm_rate = _ZERO;
		for (int i = 0; i < 8; i++) player.alpha.amount[i] = _ONE;
	}


	// Highests update.
	if (player.alpha.highest < player.pre_alpha.score) player.alpha.highest = player.pre_alpha.score;
	if (player.beta.highest < player.pre_alpha.score) player.beta.highest = player.pre_alpha.score;
	if (player.alpha.score > player.alpha.am_best) {
		player.alpha.am_best = player.alpha.score;
	}

	if (player._get_AM(6) && player.alpha.autobuyer && player.pre_alpha.score >= inf) {
		init_alpha();
	}

	// Tries to unlock Achs and Stories.
	player._try_unlock_all();

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
	save_game_state();
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
	load_game_state(tmp_path);
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
	while (pos < s.size() && (std::isdigit(s[pos]) || s[pos] == '.' || s[pos] == 'e' || s[pos] == 'E')) {
		if (s[pos] == 'e' || s[pos] == 'E') {
			++pos;
			if (pos < s.size() && (s[pos] == '-' || s[pos] == '+')) ++pos;
			while (pos < s.size() && std::isdigit(s[pos])) ++pos;
			break;
		}
		++pos;
	}
	if (pos == start) return 0;//throw std::runtime_error("Invalid number format");
	return std::stold(s.substr(start, pos - start));
}

Number read_Number(const std::string& a) {
	std::string s = a;
	size_t first = s.find_first_not_of(" \t\n\r");
	if (first == std::string::npos) return Number();
	size_t last = s.find_last_not_of(" \t\n\r");
	s = s.substr(first, last - first + 1);

	bool sign = true;
	if (s.compare(0, 2, "1/") == 0) {
		sign = false;
		s = s.substr(2);
	}

	if (s.compare(0, 2, "N(") == 0) {
		size_t pos = 0;
		Number r = parse_number(s, pos);
		if (!sign) r.sign = !r.sign;
		return r.normalize();
	}

	long double val = std::stold(s);
	if (val < 0) {
		sign = !sign;
		val = -val;
	}
	return Number(sign, val, 0.0L).normalize();
}

static Number parse_Number(const std::string& s) {
	std::string str = s;
	str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end());
	if (str.empty()) return Number();

	if (str.compare(0, 2, "N(") == 0) {
		size_t open  = str.find('(');
		size_t close = str.rfind(')');
		if (close == std::string::npos || close <= open) return Number();
		std::string inner = str.substr(open + 1, close - open - 1);

		std::vector<std::string> parts;
		int depth = 0;
		size_t start = 0;
		for (size_t i = 0; i < inner.size(); ++i) {
			char c = inner[i];
			if (c == '(') ++depth;
			else if (c == ')') --depth;
			else if (c == ',' && depth == 0) {
				parts.push_back(inner.substr(start, i - start));
				start = i + 1;
			}
		}
		parts.push_back(inner.substr(start));

		if (parts.size() != 4) return Number();

		long double v  = std::stold(parts[0]);
		uint64_t    l  = (uint64_t)std::stoull(parts[1]);
		bool        sg = (parts[2] == "1");
		int         n  = (parts[3] == "1") ? 1 : -1;

		Number r(sg, n, v, (long double)l);
		return r.normalize();
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

void save_options() {
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

void load_options() {
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

		player.options[0].val = _ZERO;          // Mode: Seconds
		player.options[1].val = _to_Number(60); // Amount: 60s
		player.options[2].val = _to_Number(10); // Amount: 10%
		player.options[3].val_s = "save1.txt";  // Savefile name
		player.options[4].val = _to_Number(0);  // BGM Index
		player.options[5].val = _to_Number(50); // BGM Volume

		// Immediately save these defaults so they persist
		save_options();
	}
}

void save_game_state() {
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
		fprintf(save1, "%s\n", N2S_save(_to_Number(player.upgrade[i].autobuyer.enabled)).c_str());
	}

	// Save Achievements & Stories
	for (int i = 1; i <= 50; i++) {
		fprintf(save1, "ach %d\n", i);
		fprintf(save1, "%s\n", N2S_save(_to_Number(bool(player.ach[i]))).c_str());
	}
	for (int i = 1; i <= 5; i++) {
		fprintf(save1, "story %d\n", i);
		fprintf(save1, "%s\n", N2S_save(_to_Number(bool(player.story[i]))).c_str());
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
	fprintf(save1, "last_tick\n%s\n", N2S_save(_to_Number(player.last_tick)).c_str());
	fprintf(save1, "currenttab\n%s\n", N2S_save(_to_Number(player.currenttab)).c_str());
	fprintf(save1, "unlockedtabs\n%s\n", N2S_save(_to_Number(player.unlockedtabs)).c_str());
	fprintf(save1, "unlocked_prestige\n%s\n", N2S_save(_to_Number(bool(player.unlocked_prestige))).c_str());

	// Save Time Flux
	fprintf(save1, "time_flux\n%s\n", N2S_save(_to_Number(player.time_flux)).c_str());
	fprintf(save1, "flux_pool\n%s\n", N2S_save(_to_Number(flux_pool)).c_str());
	fprintf(save1, "flux_elapsed\n%s\n", N2S_save(_to_Number(flux_elapsed)).c_str());

	// Save Alpha State
	fprintf(save1, "unlocked_alpha\n%s\n", N2S_save(_to_Number(bool(player.unlocked_alpha))).c_str());
	fprintf(save1, "break_alpha\n%s\n", N2S_save(_to_Number(bool(player.break_alpha))).c_str());
	fprintf(save1, "alpha_point\n%s\n", N2S_save(player.alpha.point).c_str());
	fprintf(save1, "alpha_count\n%s\n", N2S_save(player.alpha.alpha).c_str());
	fprintf(save1, "alpha_highest\n%s\n", N2S_save(player.alpha.highest).c_str());
	fprintf(save1, "alpha_tab\n%s\n", N2S_save(_to_Number(player.alpha.tab)).c_str());
	fprintf(save1, "alpha_auto\n%s\n", N2S_save(_to_Number(player.alpha.autobuyer)).c_str());
	fprintf(save1, "fastest_ap\n%s\n", N2S_save(player.fastest_ap_per_min).c_str());

	fprintf(save1, "alpha_score\n%s\n", N2S_save(player.alpha.score).c_str());
	fprintf(save1, "alpha_am_best\n%s\n", N2S_save(player.alpha.am_best).c_str());
	fprintf(save1, "alpha_am_rate\n%s\n", N2S_save(player.alpha.am_rate).c_str());
	fprintf(save1, "alpha_AIM\n%s\n", N2S_save(player.alpha.AIM).c_str());
	for (int i = 0; i < 8; ++i) {
		fprintf(save1, "alpha_amount %d\n%s\n", i, N2S_save(player.alpha.amount[i]).c_str());
	}

	// Save Beta game state.
	fprintf(save1, "unlocked_beta\n%s\n", N2S_save(_to_Number(bool(player.unlocked_beta))).c_str());
	fprintf(save1, "beta_point\n%s\n", N2S_save(player.beta.point).c_str());
	fprintf(save1, "beta_count\n%s\n", N2S_save(player.beta.beta).c_str());
	fprintf(save1, "beta_highest\n%s\n", N2S_save(player.beta.highest).c_str());
	fprintf(save1, "beta_tab\n%s\n", N2S_save(_to_Number(player.beta.tab)).c_str());
	fprintf(save1, "beta_auto\n%s\n", N2S_save(_to_Number(player.beta.autobuyer)).c_str());
	fprintf(save1, "fastest_bp\n%s\n", N2S_save(player.fastest_bp_per_min).c_str());

	fprintf(save1, "beta_score\n%s\n", N2S_save(player.beta.score).c_str());
	fprintf(save1, "beta_bm_best\n%s\n", N2S_save(player.beta.bm_best).c_str());
	fprintf(save1, "beta_bm_rate\n%s\n", N2S_save(player.beta.bm_rate).c_str());
	fprintf(save1, "beta_BIM\n%s\n", N2S_save(player.beta.BIM).c_str());
	fprintf(save1, "beta_highest_am\n%s\n", N2S_save(player.beta.highest_am).c_str());
	for (int i = 0; i < 8; ++i) {
		fprintf(save1, "beta_amount %d\n%s\n", i, N2S_save(player.beta.amount[i]).c_str());
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

void load_game_state(const string& filename) {
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
			init(); // Initialize new game if no save exists
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
			}


			line = read_line();
		}
	} catch (...) {
		fclose(save1);
		init(); // Reset on error
		return;


	}

	player.last_tick = _get_time();
	cost_init(); // Re-initialize costs based on loaded upgrades
	fclose(save1);
}

// Unified Read Function
void read() {
	// 1. Load Options first, because they determine the save file name
	load_options();

	// 2. Load Game State using the filename from options
	string savefile = player.options[3].val_s;
	if (savefile.empty()) savefile = "save1.txt";

	load_game_state(savefile);
}

// Unified Save Function
void save() {
	save_options();
	save_game_state();
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

		player.last_tick = tick;

		update(_to_Number(interval / 1000));

		Sleep(20);

#ifdef _WIN32
		if (g_use_gui) {
			if (!g_gui_running) break;
			GUI_RenderFrame();
		}
#endif

		if (tick - last_save_tick > 10000) {
			cost_init();
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
	cost_init();

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
