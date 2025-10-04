# AI Usage Disclosure Details

** Student Name:** Eric
** Student ID:** 5200915 (UCFID) or 031686 (NID)
** Assignment:** HW #2 (Lexical Analyzer)

---

## Instructions

Complete this template with detailed information about your AI usage. Submit this file along with your signed PDF declaration form.

---

## AI Tool #1

## Tool Name  
ChatGPT  

## Version/Model  
GPT‑5  

## Date(s) Used  
September 27th, 2025 – October 3rd, 2025 

## Specific Parts of Assignment  
I used AI assistance mainly to help me understand the conceptual background of lexical analysis and the material from lecture slides (token classification, reserved words, handling identifiers and numbers). I also asked for clarification on some debugging issues, such as how to properly handle error messages (e.g., ensuring that “number too long” and “identifier too long” appear in the lexeme table instead of being printed separately). The AI was used to help me understand why my code behaved incorrectly and to suggest a possible fix, which I then applied myself.  

## Prompts Used  
“Explain the difference between simple variables and stack-dynamic variables”
“Why is my output for long numbers not matching the format in the spec?”
“How should invalid symbols be reported in the lexeme table?”

### AI Output/Results
The AI provided concise explanations of the token classification system used in PL/0, including the difference between reserved words and identifiers. It helped confirm that whitespace and comments should be skipped entirely and not included in the lexeme table or token list. The AI also clarified how to flag specific error cases like overly long identifiers and numbers within the lexeme table itself, rather than printing them separately. Additionally, it walked through sample scanning logic for identifiers, numbers, and special symbols that aligned closely with the rules described in the HW2 PDF and lecture slides.

### How Output was Verified/Edited
I used the AI’s explanation to inform my approach but wrote the C implementation from scratch. I compiled and ran the code on Eustis using the sample programs provided in Appendix A and error cases in Appendix B of HW2. I verified that the output matched the expected format by comparing the spacing and content of the Lexeme Table and Token List to the sample outputs. I made small changes where necessary (e.g., adjusting token codes, fixing spacing, ensuring reserved words were not misclassified as identifiers) based on discrepancies between my output and the examples.


### Multiple Iterations (if applicable)
Over multiple sessions, I started by asking general questions about the scanning order and token types. Once I had the basic logic in place, I asked about how to format the Lexeme Table and Token List to match the assignment's output structure. In later iterations, I focused on edge cases—like how to detect invalid symbols or ensure that unclosed comments or numbers followed by letters were properly handled. Each time, I refined my understanding and updated my implementation accordingly.

### Learning & Reflection
I learned how to break down lexical analysis into distinct scanning steps and how to enforce precise formatting in output to match a specification. I also gained a clearer understanding of how to store and manage tokens using enums and arrays in C. The AI helped me think through tricky debugging scenarios and encouraged a more structured approach to error handling and input parsing. This process not only improved my implementation but also deepened my understanding of how compilers tokenize source code and manage lexical structure behind the scenes.

---