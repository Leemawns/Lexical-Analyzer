# AI Usage Disclosure Details

** Student Name:** Leonardo
** Student ID:** 53068862
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
I used AI mainly to help me understand how lexical analysis works and to go over the lecture slides, like how tokens, reserved words, identifiers, and numbers are handled. I also asked for help fixing small bugs, like making sure errors such as "number too long" or "identifier too long" show up correctly in the lexeme table. The AI helped explain why parts of my code didn’t work as expected and gave me ideas for how to fix them, which I then tried out and tested myself.

## Prompts Used  
"Thoughts on fread()?"
"what does this mean? (struct) const ScanCtx * S"
"so the fread, store the src code in buffer and return the size of the fiile?"

### AI Output/Results
ChatGPT explained that fread() reads raw bytes from a file into a buffer and returns how many bytes were read, which in this assignment means the entire source code is loaded into memory for scanning. 
It clarified that (struct) const ScanCtx *S is just a pointer to a constant ScanCtx struct passed without copying. 

### How Output was Verified/Edited
After clarifying how structure pointers worked, I edited my classmate’s version of the code to make the logic clearer and added my own comments for better readability. I reworded variable names, simplified conditionals, and made sure the structure access patterns aligned with the assignment.

### Multiple Iterations (if applicable)
N/A

### Learning & Reflection
I developed a better understanding of how structs are passed and manipulated in C. Became better navigating pointer-based logic and working with arrays of structs in a lexical analyzer.

---