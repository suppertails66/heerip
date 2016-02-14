/* A basic logger intended for writing errors to a file */

#include <iostream>
#include <fstream>
#include <string>

namespace ErrLog
{


	class ErrorLog
	{
	public:
		ErrorLog()
			: disabled(false), errflag(false), errcount(0),
		warnflag(false), warncount(0) { };

		bool enabled() { return !disabled; }
		bool is_open() { return ofs.is_open(); }
		void enable() { disabled = false; }
		void disable() { disabled = true; }
		bool get_errflag() { return errflag; }
		int get_errcount() { return errcount; }
		void reset_errflag() { errflag = false; }
		void reset_errcount() { errcount = 0; }
		bool get_warnflag() { return warnflag; }
		int get_warncount() { return warncount; }
		void reset_warnflag() { warnflag = false; }
		void reset_warncount() { warncount = 0; }
		void open(std::string t) 
		{ 
			if (!disabled)
			{
				target = t;
				if (ofs.is_open()) ofs.close();
				ofs.open(target.c_str(), std::ios_base::binary | std::ios_base::trunc);
			}
		}
		void appopen(std::string t) 
		{ 
			if (!disabled)
			{
				target = t;
				if (ofs.is_open()) ofs.close();
				ofs.open(target.c_str(), std::ios_base::binary | std::ios_base::app);
			}
		}
		void close() { ofs.close(); };
		std::string gettarget() { return target; }

		void print(std::string e)
		{
			if (!disabled)
			{
				std::cout << e << '\n';
				if (!ofs.is_open()) 
					return;
				ofs << e << "\r\n";
			}
		}

		void qprint(std::string e)
		{
			if (!disabled)
			{
				if (!ofs.is_open()) 
					return;
				ofs << e << "\r\n";
			}
		}

		void printnb(std::string e)
		{
			if (!disabled)
			{
				std::cout << e << '\n';
				if (!ofs.is_open()) 
					return;
				ofs << e;
			}
		}

		void qprintnb(std::string e)
		{
			if (!disabled)
			{
				if (!ofs.is_open()) 
					return;
				ofs << e;
			}
		}

		void error(std::string e)
		{
			if (!disabled)
			{
				errflag = true;
				++errcount;
				std::cout << "ERROR: " << e << '\n';
				if (!ofs.is_open()) 
					return;
				ofs << "ERROR: " << e << "\r\n";
			}
		}

		void qerror(std::string e)
		{
			if (!disabled)
			{
				errflag = true;
				++errcount;
				if (!ofs.is_open()) 
					return;
				ofs << "ERROR: " << e << "\r\n";
			}
		}

		void warning(std::string w)
		{
			if (!disabled)
			{
				warnflag = true;
				++warncount;
				std::cout << "WARNING: " << w << '\n';
				if (!ofs.is_open()) 
					return;
				ofs << "WARNING: " << w << "\r\n";
			}
		}

		void qwarning(std::string w)
		{
			if (!disabled)
			{
				warnflag = true;
				++warncount;
				if (!ofs.is_open()) 
					return;
				ofs << "WARNING: " << w << "\r\n";
			}
		}

	private:
		std::ofstream ofs;
		std::string target;
		bool disabled;
		bool errflag;
		int errcount;
		bool warnflag;
		int warncount;
	};


};	// end of namespace ErrLog

#pragma once
