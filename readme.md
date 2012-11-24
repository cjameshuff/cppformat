An extensible, type-safe approach to string formatting for C++.

Shorter, more readable code than iostream formatting, but has notable advantages over C style formatting via variadic functions as well:

* Type safety: data to be formatted is handled via overloaded functions, not variadic arguments.
* The output is determined by both the formatting operator and the data type. Formatting operators may be simplified, not needing to specify things such as signedness and size. (not currently done)
* Like iostreams, support for user-defined types can be added with a single overloaded function definition.


# Usage:

Easily used with standard output:

	cout << fstring("Testing...%d, %f, %d\n")% 1 % 2.0 % 42;

	cout << fstring("%s%c%c%c%d, %f, %d\n")% "Testing" % '.' % '.' % '.' % 1 % 2.0 % 42;


The string class can be used as well as C-style strings:

	string s = "Testing..."
	cout << fstring("%s%d, %f, %d\n")% s % 1 % 2.0 % 42;


Obtain a formatted string:

	string s = (fstring("Testing...%d, %f, %d\n")% 1 % 2.0 % 42).str();


## Extension to new types:

Here, %f is overloaded to support 4-component vectors, with specified formatting options being replicated for each vector component:

	static inline fstring operator%(const fstring & lhs, float4 rhs) {
	    std::string fs = lhs.next_fmt("f");
	    fs = (fstring("(%s, %s, %s, %s)")% fs % fs % fs % fs).str();
	    return lhs.append((fstring(fs)% rhs.x % rhs.y % rhs.z % rhs.w).str());
	}
	
	cout << (fstring("Testing...%0.3f\n")% result).str();


## Other


Use of an operator allows more flexible application of format substitutions than is possible with variable parameter lists:

	int foo[] = {1, 2, 3, 4, 5};
	fstring f("Testing...%d, %d, %d, %d, %d\n");
	for(auto i : foo)
	    f %= i;
	cout << f;

A format may also be reset to allow repeated application, at a gain in efficiency (minor at present, potentially greater), as in the following function for printing a range of values defined by a pair of iterators:

	template<typename iter_t>
	static inline std::string to_string(const std::string & fstr,
										const iter_t & begin,
										const iter_t & end,
										const std::string & sep = ", ")
	{
	    fstring fmt(fstr);
	    auto i = begin;
	    fmt %= *(i++);
	    while(i != end) {
	        fmt.reset();
	        fmt.append(sep);
	        fmt %= *(i++);
	    }
	    return fmt.str();
	}

