# Learner profile

Updated: 21 November 2026.

## Goal and approach

Understand how a variant works by implementing it, with questions and hints before full solutions. Keep the learner writing the implementation and use small checkpoints to review the reasoning.

The current implementation is `variant.cpp`. The original practice code is kept locally in `tmp/practice.cpp`, outside version control.

## Preferences

- Open-ended questions, small steps, and a different explanation when something is unclear.
- Descriptive template parameter names such as `AlternativeTypes`, `FirstType` and `InputType`.
- Keep my wording and documentation, correcting spelling and grammar without making it sound unlike me.
- Australian English, brief explanations and no em dashes in prose written on my behalf.
- Stay within the project's API before suggesting extra features.

## Demonstrated during the project

Traced recursive storage and access, distinguished runtime state from compile-time indices, and reasoned about construction versus assignment. Identified the need to destroy only the active alternative, avoid double destruction, copy into a newly constructed object, and use a temporary for different-index swap.

Implemented destruction, copying, moving, assignment, swapping and access with review and guidance. Reported successful tests during learning; these scenarios are now covered by the repository's test target.

## Topics to revisit

Forwarding references, named rvalue references, overload selection, dependent template calls, fold expressions and union lifetime rules were explained and applied with guidance. Passing tests shows the current implementation behaves as checked; it does not establish independent mastery of every concept.

## Collaboration and attribution

Codex supplied hints, examples, reviews and some syntax. At my request, it adapted the practice implementation to the final API and co-authored `test.cpp`. The README concepts came from my own notes, edited with Codex for clarity and accuracy. Continue to distinguish learner reasoning from supplied code when recording future progress.
