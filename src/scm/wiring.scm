scm
;
; wiring.scm
;
; Implements the wiring-together of opencog predicates.
;
; Copyright (c) 2008 Linas Vepstas <linasvepstas@gmail.com>
;
; Implement a constraint-type wiring system, inspired by SICP sections
; 3.3 (constraints) and 3.5 (streams), where SICP is "Structure and
; Interpretation of Computer Programs", Abelson & Sussman. Note that
; the wiring/constraint paradigm is highly amenable to graphical
; manipulation, along the lines of SGI's widely emulated and copied
; graphical wiring system from the early 1990's.
;
; The core problem that this is attempting to solve is that working
; directly with hypergraphs is hard. Fore example, problems in natural
; language processing require that some certain node in some certain
; location in the hypergraph needs to be compared to some other node
; somewhere else, and if certain conditions apply, then, as a result,
; yet another part of the hypergraph needs to be modified, deleted,
; or constructed; its truth-value needs to be modified, etc.
;
; Code that I've written so far has attacked this problem in an ad-hoc
; manner, crawling the graph, chasing link types in the forward or
; backward direction, looking for nodes of a certain type located in
; a certain place. Worse, this code is fragile: if I decide to change
; how my data (english sentences, grammatical relations, parses) are
; represented in a hypergraph, then I also need to change the code that
; crawls the graph.
;
; There is also another complication: the need for iteration. Whatever
; processing is done, it needs to be done for each word or sentence, etc.
; The ad-hoc proceedure is educational, if you've never used hypergraphs
; before, and relatively fast to implement, but its messy, and certainly
; not scalable. It gets tedious and repetitive -- one codes similar but
; slightly different things with each new algorithm that is needed. Below
; follows an experiment in doing things a different way.
;
; The core paradigm is that of "wires" or "constraints". Wires will
; connect the input or output of one routine to another. Wires carry
; values from one place to another. Consider, for example, the sentence
; "John threw a ball"; it has a relation "_subj(throw, John)" indicating
; that the subject of the verb "throw" is "John". Suppose I wanted to
; iterate over all all relations involving "John" as the subject: that
; is, I want to find all elements ?vrb that solve the predicate
; "_subj(?vrb, John)". This will be done by attaching a wire to ?vrb. The
; result is that the wire will sequentially take on all possible values
; for all matching occrurances of such triples in the hypergraph. This
; wire can be attached to various proceedures that do something with
; those values. Alternately, it might be attached to yet another predicate,
; say "_obj(?vrb, ball)", and would thus hopefully yeild all verbs where
; John did something to a ball.
;
; The design goal is to implement the wires both as constraints (as
; in SICP 3.3) so that they can carry values, and as streams (SICP 3.5)
; so to avoid infinite recursion.
;
; The point of this excercise is not to be abstract, but to explore a
; programming tool. I'm tired of writing ugly, ad-hoc, hard-to-understand
; code for crawling hypergraphs and fetching particular values out of them.
; I'm hoping that this will provide a simpler, cleaner framework.
; The results of this experiment are TBD.
; 
; Some deeper conceptual remarks:
; Of course, a wire should be understood to be a particular type of Link,
; lets say "WireLink".
;
; Implementation notes:
; By analogy to electrical engineering, can also think of wires as "buses";
; values are asserted onto the bus by one master, and all other parties
; on the bus must hold it "floating". Currently, the wire only grants 
; access to one bus-master; its might be possible to relax this.
; May need to create a specific grant/revoke protocal for the bus?
;
; Current implementation only allows *two* endpoints on the bus, not many.
; This is because the recever takes the stream from the sender. This seems
; like the most efficient way to proceed at the moment.
; The receiver is also called the "consumer", the transmitter is the "producer"
; Some of the words below refer to a bus, implying multiple endpoints, this
; no longer holds as the appropriate paradigm. -- The wire is purely 
; point-to-point.
;
; Unfortunately, ice-9 streams are significantly different than srfi-41
; streams, but, for expediancy, I'll be using ice-9 streams, for now.
; The biggest difference is that ice-9 doesn't have stream-cons, it has
; make-stream instead; so the semantics is different. Hopefully, porting 
; over won't be too hard.
;
(use-modules (ice-9 streams))
(define stream-null (make-stream (lambda (x) '()) '()))

; Simple example code:
;
; (define w (make-wire "my-wire-name"))  ; wire name used only for debugging
; (wire-probe "w-probe" w)
; (wire-source-list w (list 1 2 3 4 5))
;
; The above will create a wire w, attach a probe to the wire (the probe
; will print the values it sees on the wire) and then it will place a 
; sequence of numbers on the wire, which the probe will print out.

; Create a new wire
; Along the lines of "make-connector", SICP 3.3.5
(define (make-wire wire-dbg-name)
	(let (
		(debug-tracing #t) ; control debug tracing
		(strm stream-null) ; only be streams are allowed
		(busmaster #f)    ; "informant" in SICP
		(endpoints '()))  ; "constraints" in SICP

		; Connect a new endpoint to the bus.
		; If theres' a master on the bus, then
		; let the new endpoint know about it.
		; XXX In the current design, the downstream device
		; always "takes" the stream, so in fact, the wire
		; can have only *two* enpoints, not many. The two
		; would be the master (source) and the slave (sink)
		(define (connect new-endpoint)
			(if (not (memq new-endpoint endpoints))
				(set! endpoints (cons new-endpoint endpoints))
			)

			(if debug-tracing
				(begin
					(display "Endpoint connected on ")
					(display wire-dbg-name) (newline)
				)
			)

			; If there is a master on this wire, 
			; then tell the new endpoint about it.
			(if (stream-null? strm)
				(float-msg new-endpoint)
				(deliver-msg new-endpoint)
			)
			'done
		)

		; Give the entire stream to the receving endpoint.
		; Clear the local copy, unset the busmaster
		(define (take-stream)
			(let ((local-strm strm))
				(set! busmaster #f)
				(set! strm stream-null)
			local-strm)

			; Note that when the stream is taken, the wire is left floating.
			; However, no float-msg is sent, since presumably the taker 
			; knows about it already ... 
		)

		; Let "master" assert a value onto the bus
		(define (set-value! newval master)

			(if debug-tracing
				(begin
					(display "Set-value called on ")
					(display wire-dbg-name) (newline)
				)
			)
			(if (not (stream-null? strm)) 
				(error "Stream already set on this wire" wire-dbg-name)
			)

			; The bus is floating, so grant
			(set! strm newval)
			(set! busmaster master)
			(for-each-except master deliver-msg endpoints)
		)

		(define (me request)
			(cond 
				((eq? request 'value) (take-stream))
				((eq? request 'has-value?)
					(if busmaster #t #f)
				)
				((eq? request 'connect) connect)
				((eq? request 'set-value!) set-value!)
				(else (error "Unknown operation -- make-wire" wire-dbg-name request))
			)
		)

		; return the command dispatcher.
		me
	)
)

; return the value of the wire.
(define (wire-take-stream wire) (wire 'value))      ; SICP get-value
(define (wire-has-stream? wire) (wire 'has-value?)) ; SICP has-value?
(define (wire-set-stream! wire value endpoint)      ; SICP set-value!
	((wire 'set-value!) value endpoint)
)
(define (wire-connect wire endpoint)
	((wire 'connect) endpoint)
)

; Basic messages
; "assert" means "put a message on this bus"
; "float"  means "leave the bus in a floating (unasserted) state"
(define wire-assert-msg 'I-want-to-xmit) ; I-have-a-value in SICP
(define wire-float-msg  'Ready-to-recv)  ; I-lost-my-value in SICP

(define (deliver-msg endpoint) (endpoint wire-assert-msg))
(define (float-msg endpoint) (endpoint wire-float-msg))

; Display a stream
; This is a "consumer" endpoint, it takes the stream off the wire
; and displays the contents of the stream.  (It consumes the stream).
;
; XXX Need to have a passive probe that can snoop on the wire.
; XXX this would probably have to be a special, snooping-wire.
;
(define (wire-probe probe-name wire)
	(define (prt-val value)
		(display "Probe: " )
		(display probe-name)
		(display " = ")
		(display value)
		(newline)
	)

	(define (me request)
		(cond 
			((eq? request wire-assert-msg)
				(stream-for-each prt-val (wire-take-stream wire)) 
			)
			((eq? request wire-float-msg)
				(prt-val "floating") 
			)
			(else
				(error "unkonwn request -- wire-probe" request)
			)
		)
	)

	; hook me up
	(wire-connect wire me)

	; return the command dispatcher.
	me
)

; Place a clock source on a wire
; This generates an infinite sequence of alternating #t, #f values
;
; Do *not* use this with the current opencog scheme interpreter.
; The problem is that this goes into an infinite loop. Worse,
; the output is buffered, and is invisible. Worse still, there's
; no way to ctrl-C this, since the shell server isn't properly
; threaded for i/o. Bummer. Should be fixed someday.
(define (clock-source wire)
	(define (toggle state) (cons state (not state)))

	(define (me request)
		(cond 
			((eq? request wire-float-msg) (lambda () #f)) ; ignore float message
			(else (error "Unknown request -- clock-source" request))
		)
	)

	(wire-connect wire me)
	(wire-set-stream! wire (make-stream toggle #f) me)

	; return the command dispatcher.
	me
)

; Place a list onto a wire
; This creates a bus-master; the list will be clocked onto the bus.
(define (wire-source-list wire lst)

	(define (me request)
		(cond 


			((eq? request wire-float-msg) (lambda () #f)) ; ignore float message
			(else (error "Unknown request -- list-source" request))
		)
	)

	(wire-connect wire me)
	(wire-set-stream! wire (list->stream lst) me)

	; return the command dispatcher.
	me
)
.
exit
