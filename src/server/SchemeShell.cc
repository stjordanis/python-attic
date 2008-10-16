/*
 * SchemeShell.c
 *
 * Simple scheme shell
 * Copyright (c) 2008 Linas Vepstas <linas@linas.org>
 */

#ifdef HAVE_GUILE

#include "platform.h"
#include "SchemeShell.h"

using namespace opencog;

// Some random RFC 854 characters
#define IAC 0xff  // Telnet Interpret As Command
#define IP 0xf4   // Telnet IP Interrupt Process
#define AO 0xf5   // Telnet AO Abort Output
#define EL 0xf8   // Telnet EL Erase Line
#define WILL 0xfb // Telnet WILL
#define DO 0xfd   // Telnet DO
#define TIMING_MARK 0x6 // Telnet RFC 860 timing mark

SchemeShell::SchemeShell(void)
{
	show_output = true;
	normal_prompt = "guile> ";
	pending_prompt = "... ";
	abort_prompt = "asdf";
	abort_prompt[0] = IAC;
	abort_prompt[1] = WILL;
	abort_prompt[2] = TIMING_MARK;
	abort_prompt[3] = '\n';
	abort_prompt += normal_prompt;
}

void SchemeShell::hush_output(bool hush)
{
	show_output = !hush;
}

const std::string& SchemeShell::get_prompt(void)
{
	// Use different prompts, depending on whether there is pending
	// input or not.
	if (evaluator.input_pending())
	{
		return pending_prompt;
	}
	else
	{
		return normal_prompt;
	}
}

/* ============================================================== */

/**
 * Evaluate the expression
 */
std::string SchemeShell::eval(const std::string &expr)
{
	size_t len = expr.length();
	if (0 == len) 
	{
		return get_prompt();
	}

	// Handle Telnet RFC 854 IAC format
	// Basically, we're looking for telnet-encoded abort or interrupt
	// characters, starting at the end of the input string. If they
	// are there, then don't process input, and clar out the evaluator.
	// Also, be sure to send telnet IAC WILL TIMING-MARK so that telnet
	// doesn't sit there flushing output forever.
	//
	// Search for IAC to at most 20 chars from the end of the string.
	int i = len-2;
	int m = len - 20;
	if (m < 0) m = 0;
	while (m <= i)
	{
		unsigned char c = expr[i];
		if (IAC == c)
		{
			c = expr[i+1];
			if ((IP == c) || (AO == c))
			{
				evaluator.clear_pending();
				return abort_prompt;
			}

			// Erase line -- just ignore this line.
			if (EL == c)
			{
				return get_prompt();
			}
		}
		i--;
	}

	// Don't evaluate if the line is terminated by 
	// escape (^[), cancel (^X) or quit (^C)
	// These would typically be sent by netcat, and not telnet.
	unsigned char c = expr[len-1];
	if ((0x16 == c) || (0x18 == c) || (0x1b == c))
	{
		evaluator.clear_pending();
		return "\n" + normal_prompt;
	}


	/* The #$%^& opecog command shell processor cuts off the 
	 * newline character. Re-insert it; otherwise, comments within
	 * procedures will have the effect of commenting out the rest
	 * of the procedure, leading to garbage.
	 *
	 * (This is a pointless string copy, it should be eliminated)
	 */
	std::string input = expr + "\n";

	std::string result = evaluator.eval(input.c_str());

	if (evaluator.input_pending())
	{
		if (show_output)
			return pending_prompt;
		else
			return "";
	}

	if (show_output || evaluator.eval_error())
	{
		result += normal_prompt;
		return result;
	}
	else
	{
		return "";
	}
	return "#<Error: Unreachable statement reached>";
}

#endif
/* ===================== END OF FILE ============================ */
