# kfre -- Flesch-Kincard readability ease score, and other readability metrics

`fkre` is a simple utility that displays the Flesch-Kincaid score and
other readability metrics, for English text. It has an HTML mode that
can exclude HTML tags that would otherwise confuse the metrics. In
HTML mode the utility can also count the number of words in each
subheading, allowing the detection of over-long sections.

`fkre` estimates the number of passive-voice expressions, which many
writes like to avoid where practicable. 

FK scores are frequently used by web-authoring and search-engine
optimization tools, as well as some word processors. I was surprised
that there did not seem to be a command-line utility for Linux, so
I wrote one.

## Building

`fkre` was written for Linux, and is designed to be compiled using the
GNU C compiler. In general:

    % make 
    % sudo make install

## Usage

    fkre [--html] [--version] {filename}


## The Flesch-Kincaid score

The FK readability ease score is calculated as

    206.835 - 1.015 * words_per_sentence - 84.6 * syllables_per_word.

Most English texts have scores in the range 10-100, although both
higher and lower scores can be contrived. The score is
not a percentage. For example, "A cat sat on a mat"
scores 116. In practice, most users of the FK score aim for
values of 60 or higher when writing for a general readership. 
Specialist and scientific publications might be considered quite
accessible by their readers with an FK score higher than 30.  

Note that the FK score considers only sentence length and word
structure. Other factors also affect readability, such as the
use of subheadings. `fkre` reports some of these metrics separately,
if sufficient information is available. 

## Notes

1. A sentence is a group of words ending with '.' or '?' -- no other
line ending is recognized. In particular, HTML line and paragraph breaks
are not taken, by themselves, to mark the end of a sentence.

2. The algorithm considers English letters, and a small number of non-English
letters that frequently appear in English text. The FK algorithm is 
generally only meaningful -- to the extent that it is meaningful at all --
for the English language.

3. `fkre` understands ASCII and UTF8 encodings only. 

4. Many factors will interfere with the calculation. For example, there is 
no general agreement on how abbreviations and acronyms affect readability.
An abbreviation with no vowels is treated as a word with no syllables; with
vowels it may be read as an ordinary word. Text with large numbers of
abbreviations may be considered difficult to read, but the FK algorithm 
takes no account of this. 

5. As a word-counter, `fkre` is more subtle than the standard `wc` utility
(although much slower). `wc` treats any group of symbols separated by
white space as a word, but `fkre` excludes numbers and non-alphabetic
symbols.

## Limitations

`fkre` favours speed and simplicity over accuracy. Part of the algorithm
involves breaking words down into syllables, which is intrinsically
difficult. `fkre` uses a simple algorithm that just looks for groups of
consonants interspersed with groups of vowels. This does tend slightly
to under-estimate the number of syllables and thus increase the FK
readability score. However, this effect is only apparent with short
texts. 

At present, `fkre` does not actually parse HTML -- it just strips tags.
Some non-tag HTML formatting might still be counted as words. 
Most likely a long HTML table will just appear to be a very long sentence,
which will increase the average sentence length and thus decrease the
readability score. These effects become more significant with shorter
documents, as there is less opportunity for anomalies to be averaged
away.

The way `fkre` detects passive voice expressions is fairly simplistic --
it only counts 'to be' verbs followed immediately by a participle. So
"was placed" will count, but "was often placed" will not. Irregular
past participles ("the program was run") aren't counted, either. 

## Author and copyright

`fkre` is maintained by Kevin Boone and distributed under the terms of
the GNU Public Licence v3.0. I wrote it for my own use and I'm making it
available in the hope that it will be useful to other people. However, 
there is no warranty of any kind.

