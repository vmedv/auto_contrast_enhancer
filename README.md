# Auto Contrast Enhancer

This program can enhance contrast of image (PPM and PGM formats: 'P5' and 'P6' filetypes)

**Usage:** `./contrast-enhancer <number_of_threads> <input_file_name> <output_file_name> <coeff>`

`<coeff>` is a coefficient stands for part of colors (float number that satisfies `0 <= coeff < 0.5`) which should be ignored from bottom and from top. For example, program should ignore noise pixels.

Files `snail2.pnm` and `rays2.pnm` are provided for testing.
