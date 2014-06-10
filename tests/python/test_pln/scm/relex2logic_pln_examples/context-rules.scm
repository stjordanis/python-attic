; Examples to test the contextualize and decontextualize rules
; using the pattern matcher
; The rules can be loaded from opencog/learning/RuleEngine/rules/pln
; together with this input data.

(EvaluationLink
    (PredicateNode "inputs")
    (ListLink
        ; 1) input for (pln-rule-contextualize-inheritance)
        ; "Ben is competent in doing mathematics."
        (InheritanceLink
            (AndLink
                (ConceptNode "Ben")
                (ConceptNode "doing_mathematics"))
            (AndLink
                (ConceptNode "competent")
                (ConceptNode "doing_mathematics")))
        ; 2) input for (pln-rule-contextualize-evaluation)
        ; "The sky is blue in the context of the earth.
        ; The sky is not blue in the context of the moon."
        (EvaluationLink (stv .8 .7)
            (PredicateNode "isBlue")
            (AndLink
                (ConceptNode "sky")
                (ConceptNode "earth")))
        (EvaluationLink (stv .0 .2)
            (PredicateNode "isBlue")
            (AndLink
                (ConceptNode "sky")
                (ConceptNode "moon")))
        ; 3) input for (pln-rule-contextualize-subset)
        ; "Dogs are animals"
        (SubsetLink
            (ConceptNode "dogs")
            (ConceptNode "animals"))))

(define check-output
    (BindLink
        (ImplicationLink
            (AndLink
                ; 1) output
                (ContextLink
                    (ConceptNode "doing_mathematics")
                    (InheritanceLink
                        (ConceptNode "Ben")
                        (ConceptNode "competent")))
                ; 2) output 
                (ContextLink
                    (ConceptNode "earth")
                    (EvaluationLink (stv .8 .7)
                        (PredicateNode "isBlue")
                        (ListLink
                            (ConceptNode "sky"))))
                (ContextLink 
                    (ConceptNode "moon")
                    (EvaluationLink (stv .0 .2)
                        (PredicateNode "isBlue")
                        (ListLink
                            (ConceptNode "sky"))))
                ; 3) output
                (ContextLink
                    (ConceptNode "dogs")
                    (ConceptNode "animals")))
            (ConceptNode "true"))))
