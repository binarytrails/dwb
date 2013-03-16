(function () {
    Object.defineProperties(util, 
    { 
        /** 
         * Get the selected text in a webview
         * @name getSelection
         * @memberOf util
         * @function
         *
         * @returns {String} The selected text or null if no text was selected
         * */
        "getSelection" : 
        {
            value : function() 
            {
                var frames = tabs.current.allFrames;
                for (var i=frames.length-1; i>=0; --i) 
                {
                    var selection = JSON.parse(frames[i].inject("return document.getSelection().toString()"));
                    if (selection.length > 0)
                        return selection;
                }
                return null;
            }
        },
        /** 
         * Converts camel-case string for usage with GObject properties to a
         * non-camel-case String
         * @name uncamelize 
         * @memberOf util 
         * @function
         * @example 
         * util.uncamelize("fooBarBaz"); // "foo-bar-baz"
         *
         * @param {String} text The text to uncamelize
         *
         * @returns {String} The uncamelized String
         * 
         * */
        "uncamelize" : 
        {
            value : function(text) 
            {
                if (! text || text.length === 0)
                    return text;
                return text.replace(/(.)?([A-Z])/g, function(x, s, m) { 
                    return s ? s + "-" + m.toLowerCase() : m.toLowerCase(); 
                });
            }
        },
        /** 
         * Converts a non-camel-case string to a camel-case string
         *
         * @name camelize 
         * @memberOf util 
         * @function
         * @example 
         * util.camelize("foo-bar-baz"); // "fooBarBaz"
         *
         * @param {String} text The text to camelize
         *
         * @returns {String} A camelcase String
         * */
        "camelize" : 
        {
            value : function(text) 
            {
                if (! text || text.length === 0)
                    return text;
                return text.replace(/[-_]+(.)?/g, function(a, b) { 
                    return b ? b.toUpperCase() : ""; 
                });
            }
        },
        /**
         * Mixes properties of objects into an object. Properties are mixed in
         * from left to right, so properties will not be overwritten in the
         * leftmost object if they are already defined. 
         *
         * @name mixin 
         * @memberOf util 
         * @function 
         *
         * @param {Object} self 
         *      The object to mixin the properties
         * @param {...Object} varargs 
         *      Variable number of objects to mix in.
         *
         * @returns {Object}
         *      <i>self</i> or a new object if <i>self</i> is null or undefined.
         *
         * @example 
         * var a = { a : 1, b : 2, c : 3 };
         * var b = { b : 1, d : 2, e : 3 };
         * var c = { e : 1, f : 2, g : 3 };
         * 
         * a = util.mixin(a, b, c); // a = { a : 1, b : 2, c : 3, d : 2, e : 3, f : 2, g : 3}
         * */
        "mixin" : 
        {
            value : function(self)
            {
                var i, l, key, o;
                self = self || {};
                for (i=1, l=arguments.length; i<l; i++)
                {
                    o = arguments[i];
                    for (key in o)
                    {
                        if (!self.hasOwnProperty(key)) 
                        {
                            self[key] = o[key];
                        }
                    }
                }
                return self;
            }
        }
    });
    Object.freeze(util);
    
    if (Object.prototype.forEach === undefined) 
    {
        Object.defineProperty(Object.prototype, "forEach", 
        { 
            value : function (callback) 
            {
                var key;
                for (key in this) 
                {
                    if (this.hasOwnProperty(key))
                        callback(key, this[key], this); 
                }
            }
        });
    }
})();
