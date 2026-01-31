import java.util.regex.*;
import java.util.ArrayList;
import java.util.List;

/**
 * _sre - C-extension replacement for Python's regex engine.
 * 
 * Wraps Java's java.util.regex to provide Python re module functionality.
 * Most Python regex syntax is compatible with Java's, but some conversions are needed.
 */
public class _sre {
    
    // Flag constants (matching Python's re module)
    public static final int IGNORECASE = 2;
    public static final int I = IGNORECASE;
    public static final int LOCALE = 4;
    public static final int L = LOCALE;
    public static final int MULTILINE = 8;
    public static final int M = MULTILINE;
    public static final int DOTALL = 16;
    public static final int S = DOTALL;
    public static final int UNICODE = 32;
    public static final int U = UNICODE;
    public static final int VERBOSE = 64;
    public static final int X = VERBOSE;
    public static final int ASCII = 256;
    public static final int A = ASCII;
    
    /**
     * Compile a regular expression pattern.
     */
    public static Pattern compile($O pattern) {
        return compile(pattern, $I.of(0));
    }
    
    public static Pattern compile($O pattern, $O flags) {
        String p = (($S)pattern).value;
        int f = flags instanceof $I ? (int)(($I)flags).value : 0;
        return new Pattern(p, f);
    }
    
    /**
     * Search for pattern anywhere in string.
     */
    public static $O search($O pattern, $O string) {
        return search(pattern, string, $I.of(0));
    }
    
    public static $O search($O pattern, $O string, $O flags) {
        Pattern p = pattern instanceof Pattern ? (Pattern)pattern : compile(pattern, flags);
        return p.search(string);
    }
    
    /**
     * Match pattern at start of string.
     */
    public static $O match($O pattern, $O string) {
        return match(pattern, string, $I.of(0));
    }
    
    public static $O match($O pattern, $O string, $O flags) {
        Pattern p = pattern instanceof Pattern ? (Pattern)pattern : compile(pattern, flags);
        return p.match(string);
    }
    
    /**
     * Match entire string against pattern.
     */
    public static $O fullmatch($O pattern, $O string) {
        return fullmatch(pattern, string, $I.of(0));
    }
    
    public static $O fullmatch($O pattern, $O string, $O flags) {
        Pattern p = pattern instanceof Pattern ? (Pattern)pattern : compile(pattern, flags);
        return p.fullmatch(string);
    }
    
    /**
     * Find all non-overlapping matches.
     */
    public static $L findall($O pattern, $O string) {
        return findall(pattern, string, $I.of(0));
    }
    
    public static $L findall($O pattern, $O string, $O flags) {
        Pattern p = pattern instanceof Pattern ? (Pattern)pattern : compile(pattern, flags);
        return p.findall(string);
    }
    
    /**
     * Return iterator of match objects.
     */
    public static $O finditer($O pattern, $O string) {
        return finditer(pattern, string, $I.of(0));
    }
    
    public static $O finditer($O pattern, $O string, $O flags) {
        Pattern p = pattern instanceof Pattern ? (Pattern)pattern : compile(pattern, flags);
        return p.finditer(string);
    }
    
    /**
     * Substitute pattern matches with replacement.
     */
    public static $S sub($O pattern, $O repl, $O string) {
        return sub(pattern, repl, string, $I.of(0), $I.of(0));
    }
    
    public static $S sub($O pattern, $O repl, $O string, $O count) {
        return sub(pattern, repl, string, count, $I.of(0));
    }
    
    public static $S sub($O pattern, $O repl, $O string, $O count, $O flags) {
        Pattern p = pattern instanceof Pattern ? (Pattern)pattern : compile(pattern, flags);
        return p.sub(repl, string, count);
    }
    
    /**
     * Split string by pattern.
     */
    public static $L split($O pattern, $O string) {
        return split(pattern, string, $I.of(0), $I.of(0));
    }
    
    public static $L split($O pattern, $O string, $O maxsplit) {
        return split(pattern, string, maxsplit, $I.of(0));
    }
    
    public static $L split($O pattern, $O string, $O maxsplit, $O flags) {
        Pattern p = pattern instanceof Pattern ? (Pattern)pattern : compile(pattern, flags);
        return p.split(string, maxsplit);
    }
    
    /**
     * Escape special regex characters.
     */
    public static $S escape($O pattern) {
        String p = (($S)pattern).value;
        return $S.of(java.util.regex.Pattern.quote(p));
    }
    
    /**
     * Convert Python regex syntax to Java.
     */
    private static String convertPattern(String pattern) {
        // Python uses (?P<name>...) for named groups, Java uses (?<name>...)
        pattern = pattern.replaceAll("\\(\\?P<([^>]+)>", "(?<$1>");
        
        // Python uses (?P=name) for backreferences, Java uses \k<name>
        pattern = pattern.replaceAll("\\(\\?P=([^)]+)\\)", "\\\\k<$1>");
        
        return pattern;
    }
    
    /**
     * Convert Python flags to Java Pattern flags.
     */
    private static int convertFlags(int pyFlags) {
        int javaFlags = 0;
        if ((pyFlags & IGNORECASE) != 0) javaFlags |= java.util.regex.Pattern.CASE_INSENSITIVE;
        if ((pyFlags & MULTILINE) != 0) javaFlags |= java.util.regex.Pattern.MULTILINE;
        if ((pyFlags & DOTALL) != 0) javaFlags |= java.util.regex.Pattern.DOTALL;
        if ((pyFlags & UNICODE) != 0) javaFlags |= java.util.regex.Pattern.UNICODE_CASE;
        if ((pyFlags & VERBOSE) != 0) javaFlags |= java.util.regex.Pattern.COMMENTS;
        return javaFlags;
    }
    
    /**
     * Compiled regular expression pattern.
     */
    public static class Pattern extends $O {
        public final $S pattern;
        public final $I flags;
        private final java.util.regex.Pattern javaPattern;
        private final int groupCount;
        private final List<String> groupNames;
        
        public Pattern(String pattern, int flags) {
            this.pattern = $S.of(pattern);
            this.flags = $I.of(flags);
            
            String converted = convertPattern(pattern);
            int javaFlags = convertFlags(flags);
            
            try {
                this.javaPattern = java.util.regex.Pattern.compile(converted, javaFlags);
            } catch (PatternSyntaxException e) {
                throw new $X("re.error", "bad pattern: " + e.getMessage());
            }
            
            this.groupCount = javaPattern.matcher("").groupCount();
            
            // Extract named groups
            this.groupNames = new ArrayList<>();
            java.util.regex.Matcher m = java.util.regex.Pattern.compile("\\(\\?<([^>]+)>").matcher(converted);
            while (m.find()) {
                groupNames.add(m.group(1));
            }
        }
        
        /**
         * Search for pattern anywhere in string.
         */
        public $O search($O string) {
            String s = (($S)string).value;
            Matcher m = javaPattern.matcher(s);
            if (m.find()) {
                return new Match(this, s, m);
            }
            return $N.INSTANCE;
        }
        
        /**
         * Match pattern at start of string.
         */
        public $O match($O string) {
            String s = (($S)string).value;
            Matcher m = javaPattern.matcher(s);
            if (m.lookingAt()) {
                return new Match(this, s, m);
            }
            return $N.INSTANCE;
        }
        
        /**
         * Match entire string.
         */
        public $O fullmatch($O string) {
            String s = (($S)string).value;
            Matcher m = javaPattern.matcher(s);
            if (m.matches()) {
                return new Match(this, s, m);
            }
            return $N.INSTANCE;
        }
        
        /**
         * Find all non-overlapping matches.
         */
        public $L findall($O string) {
            String s = (($S)string).value;
            Matcher m = javaPattern.matcher(s);
            $L result = new $L();
            
            while (m.find()) {
                if (groupCount == 0) {
                    // No groups - return matched string
                    result.append($S.of(m.group()));
                } else if (groupCount == 1) {
                    // One group - return group content
                    String g = m.group(1);
                    result.append(g != null ? $S.of(g) : $S.of(""));
                } else {
                    // Multiple groups - return tuple
                    $O[] groups = new $O[groupCount];
                    for (int i = 0; i < groupCount; i++) {
                        String g = m.group(i + 1);
                        groups[i] = g != null ? $S.of(g) : $S.of("");
                    }
                    result.append($T.of(groups));
                }
            }
            
            return result;
        }
        
        /**
         * Return iterator of match objects.
         */
        public $O finditer($O string) {
            String s = (($S)string).value;
            return new MatchIterator(this, s);
        }
        
        /**
         * Substitute pattern matches.
         */
        public $S sub($O repl, $O string, $O count) {
            String s = (($S)string).value;
            int maxCount = count instanceof $I ? (int)(($I)count).value : 0;
            
            if (repl instanceof $S) {
                // Simple string replacement
                String replacement = (($S)repl).value;
                // Convert Python backreferences (\1, \g<name>) to Java ($1, ${name})
                replacement = replacement.replaceAll("\\\\(\\d+)", "\\$$1");
                replacement = replacement.replaceAll("\\\\g<([^>]+)>", "\\${$1}");
                
                Matcher m = javaPattern.matcher(s);
                StringBuffer sb = new StringBuffer();
                int replaced = 0;
                
                while (m.find()) {
                    if (maxCount > 0 && replaced >= maxCount) break;
                    m.appendReplacement(sb, replacement);
                    replaced++;
                }
                m.appendTail(sb);
                
                return $S.of(sb.toString());
            } else {
                // Function replacement
                Matcher m = javaPattern.matcher(s);
                StringBuffer sb = new StringBuffer();
                int replaced = 0;
                
                while (m.find()) {
                    if (maxCount > 0 && replaced >= maxCount) break;
                    Match match = new Match(this, s, m);
                    $O result = repl.__call__(match);
                    m.appendReplacement(sb, Matcher.quoteReplacement((($S)result).value));
                    replaced++;
                }
                m.appendTail(sb);
                
                return $S.of(sb.toString());
            }
        }
        
        /**
         * Split string by pattern.
         */
        public $L split($O string, $O maxsplit) {
            String s = (($S)string).value;
            int max = maxsplit instanceof $I ? (int)(($I)maxsplit).value : 0;
            
            $L result = new $L();
            Matcher m = javaPattern.matcher(s);
            int lastEnd = 0;
            int splits = 0;
            
            while (m.find()) {
                if (max > 0 && splits >= max) break;
                
                result.append($S.of(s.substring(lastEnd, m.start())));
                
                // Include captured groups
                for (int i = 1; i <= groupCount; i++) {
                    String g = m.group(i);
                    result.append(g != null ? $S.of(g) : $N.INSTANCE);
                }
                
                lastEnd = m.end();
                splits++;
            }
            
            result.append($S.of(s.substring(lastEnd)));
            return result;
        }
        
        @Override
        public $O __getattr__(String name) {
            final Pattern self = this;
            switch (name) {
                case "pattern": return pattern;
                case "flags": return flags;
                case "groups": return $I.of(groupCount);
                case "groupindex":
                    $D idx = new $D();
                    for (int i = 0; i < groupNames.size(); i++) {
                        idx.__setitem__($S.of(groupNames.get(i)), $I.of(i + 1));
                    }
                    return idx;
                case "search": return new $O() {
                    @Override public $O __call__($O... args) { return self.search(args[0]); }
                };
                case "match": return new $O() {
                    @Override public $O __call__($O... args) { return self.match(args[0]); }
                };
                case "fullmatch": return new $O() {
                    @Override public $O __call__($O... args) { return self.fullmatch(args[0]); }
                };
                case "findall": return new $O() {
                    @Override public $O __call__($O... args) { return self.findall(args[0]); }
                };
                case "finditer": return new $O() {
                    @Override public $O __call__($O... args) { return self.finditer(args[0]); }
                };
                case "sub": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return self.sub(args[0], args[1], args.length > 2 ? args[2] : $I.of(0)); 
                    }
                };
                case "split": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return self.split(args[0], args.length > 1 ? args[1] : $I.of(0)); 
                    }
                };
                default: return super.__getattr__(name);
            }
        }
        
        @Override
        public $S __repr__() {
            return $S.of("re.compile(" + pattern.__repr__().value + ")");
        }
    }
    
    /**
     * Match object - result of a successful match.
     */
    public static class Match extends $O {
        private final Pattern re;
        private final String string;
        private final int start;
        private final int end;
        private final String[] groups;
        private final int[] groupStarts;
        private final int[] groupEnds;
        
        public Match(Pattern re, String string, Matcher m) {
            this.re = re;
            this.string = string;
            this.start = m.start();
            this.end = m.end();
            
            int count = m.groupCount();
            this.groups = new String[count + 1];
            this.groupStarts = new int[count + 1];
            this.groupEnds = new int[count + 1];
            
            groups[0] = m.group();
            groupStarts[0] = m.start();
            groupEnds[0] = m.end();
            
            for (int i = 1; i <= count; i++) {
                groups[i] = m.group(i);
                try {
                    groupStarts[i] = m.start(i);
                    groupEnds[i] = m.end(i);
                } catch (IllegalStateException e) {
                    groupStarts[i] = -1;
                    groupEnds[i] = -1;
                }
            }
        }
        
        /**
         * Get matched group.
         */
        public $O group($O... args) {
            if (args.length == 0) {
                return $S.of(groups[0]);
            }
            
            if (args.length == 1) {
                int idx = getGroupIndex(args[0]);
                String g = groups[idx];
                return g != null ? $S.of(g) : $N.INSTANCE;
            }
            
            // Multiple args - return tuple
            $O[] result = new $O[args.length];
            for (int i = 0; i < args.length; i++) {
                int idx = getGroupIndex(args[i]);
                String g = groups[idx];
                result[i] = g != null ? $S.of(g) : $N.INSTANCE;
            }
            return $T.of(result);
        }
        
        private int getGroupIndex($O arg) {
            if (arg instanceof $I) {
                return (int)(($I)arg).value;
            } else if (arg instanceof $S) {
                String name = (($S)arg).value;
                // Look up named group
                for (int i = 0; i < re.groupNames.size(); i++) {
                    if (re.groupNames.get(i).equals(name)) {
                        return i + 1;
                    }
                }
                throw new $X.IndexError("no such group: " + name);
            }
            throw new $X.TypeError("group index must be int or str");
        }
        
        /**
         * Get all groups as tuple.
         */
        public $T groups() {
            return groups($N.INSTANCE);
        }
        
        public $T groups($O defaultVal) {
            $O[] result = new $O[groups.length - 1];
            for (int i = 1; i < groups.length; i++) {
                result[i - 1] = groups[i] != null ? $S.of(groups[i]) : defaultVal;
            }
            return $T.of(result);
        }
        
        /**
         * Get named groups as dict.
         */
        public $D groupdict() {
            return groupdict($N.INSTANCE);
        }
        
        public $D groupdict($O defaultVal) {
            $D result = new $D();
            for (int i = 0; i < re.groupNames.size(); i++) {
                String name = re.groupNames.get(i);
                String value = groups[i + 1];
                result.__setitem__($S.of(name), value != null ? $S.of(value) : defaultVal);
            }
            return result;
        }
        
        /**
         * Get start position of match or group.
         */
        public $I start($O... args) {
            int idx = args.length > 0 ? getGroupIndex(args[0]) : 0;
            return $I.of(groupStarts[idx]);
        }
        
        /**
         * Get end position of match or group.
         */
        public $I end($O... args) {
            int idx = args.length > 0 ? getGroupIndex(args[0]) : 0;
            return $I.of(groupEnds[idx]);
        }
        
        /**
         * Get (start, end) tuple.
         */
        public $T span($O... args) {
            int idx = args.length > 0 ? getGroupIndex(args[0]) : 0;
            return $T.of($I.of(groupStarts[idx]), $I.of(groupEnds[idx]));
        }
        
        /**
         * Expand replacement template.
         */
        public $S expand($O template) {
            String t = (($S)template).value;
            // Convert backreferences
            for (int i = groups.length - 1; i >= 0; i--) {
                String g = groups[i] != null ? groups[i] : "";
                t = t.replace("\\" + i, g);
            }
            return $S.of(t);
        }
        
        @Override
        public $O __getattr__(String name) {
            final Match self = this;
            switch (name) {
                case "re": return re;
                case "string": return $S.of(string);
                case "pos": return $I.of(0);
                case "endpos": return $I.of(string.length());
                case "lastindex": 
                    for (int i = groups.length - 1; i >= 1; i--) {
                        if (groups[i] != null) return $I.of(i);
                    }
                    return $N.INSTANCE;
                case "lastgroup":
                    for (int i = groups.length - 1; i >= 1; i--) {
                        if (groups[i] != null && i <= re.groupNames.size()) {
                            return $S.of(re.groupNames.get(i - 1));
                        }
                    }
                    return $N.INSTANCE;
                case "group": return new $O() {
                    @Override public $O __call__($O... args) { return self.group(args); }
                };
                case "groups": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return args.length > 0 ? self.groups(args[0]) : self.groups(); 
                    }
                };
                case "groupdict": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return args.length > 0 ? self.groupdict(args[0]) : self.groupdict(); 
                    }
                };
                case "start": return new $O() {
                    @Override public $O __call__($O... args) { return self.start(args); }
                };
                case "end": return new $O() {
                    @Override public $O __call__($O... args) { return self.end(args); }
                };
                case "span": return new $O() {
                    @Override public $O __call__($O... args) { return self.span(args); }
                };
                case "expand": return new $O() {
                    @Override public $O __call__($O... args) { return self.expand(args[0]); }
                };
                default: return super.__getattr__(name);
            }
        }
        
        @Override
        public $S __repr__() {
            return $S.of("<re.Match object; span=(" + start + ", " + end + "), match=" + 
                $S.of(groups[0]).__repr__().value + ">");
        }
        
        @Override
        public boolean __bool__() {
            return true;  // Match objects are always truthy
        }
        
        // Support indexing like m[0], m[1], m["name"]
        @Override
        public $O __getitem__($O key) {
            return group(key);
        }
    }
    
    /**
     * Iterator for finditer().
     */
    static class MatchIterator extends $O {
        private final Pattern pattern;
        private final String string;
        private final Matcher matcher;
        
        MatchIterator(Pattern pattern, String string) {
            this.pattern = pattern;
            this.string = string;
            this.matcher = pattern.javaPattern.matcher(string);
        }
        
        @Override
        public $O __iter__() {
            return this;
        }
        
        @Override
        public $O __next__() {
            if (matcher.find()) {
                return new Match(pattern, string, matcher);
            }
            throw new $X.StopIteration();
        }
    }
    
    /**
     * Get module attribute.
     */
    public static $O getAttr(String name) {
        switch (name) {
            case "IGNORECASE": case "I": return $I.of(IGNORECASE);
            case "LOCALE": case "L": return $I.of(LOCALE);
            case "MULTILINE": case "M": return $I.of(MULTILINE);
            case "DOTALL": case "S": return $I.of(DOTALL);
            case "UNICODE": case "U": return $I.of(UNICODE);
            case "VERBOSE": case "X": return $I.of(VERBOSE);
            case "ASCII": case "A": return $I.of(ASCII);
            default: return null;
        }
    }
}
