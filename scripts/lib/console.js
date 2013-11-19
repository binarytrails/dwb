(function() {
    /**
     * @name assert
     * @memberOf console
     * @function
     * @param {Expression} Expression to test
     * */
    /**
     * @name count
     * @memberOf console
     * @function
     * @param {String} Name
     * */
    /**
     * @name debug
     * @memberOf console
     * @function
     * @param {Object} argument Argument passed to real function
     * */
    /**
     * @name error
     * @memberOf console
     * @function
     * @param {Object} argument Argument passed to real function
     * */
    /**
     * @name group
     * @memberOf console
     * @function
     * @param {Object} argument Argument passed to real function
     * */
    /**
     * @name groupCollapsed
     * @memberOf console
     * @function
     * @param {Object} argument Argument passed to real function
     * */
    /**
     * @name groupEnd
     * @memberOf console
     * @function
     * @param {Object} argument Argument passed to real function
     * */
    /**
     * @name info
     * @memberOf console
     * @function
     * @param {Object} argument Argument passed to real function
     * */
    /**
     * @name log
     * @memberOf console
     * @function
     * @param {Object} argument Argument passed to real function
     * */
    /**
     * @name time
     * @memberOf console
     * @function
     * @param {String} name Argument passed to real function
     * */
    /**
     * @name timeEnd
     * @memberOf console
     * @function
     * @param {String} name Argument passed to real function
     * */
    /**
     * @name warn
     * @memberOf console
     * @function
     * @param {Object} argument Argument passed to real function
     * */
    function execute(f, arg)
    {
        tabs.current.inject("console." + f + ".apply(console, JSON.parse(arguments[0]));", 
            JSON.stringify(Array.prototype.slice.call(arguments, 1)));
    }
    var i, method;
    var o = {};
    var methods = [ 
        "assert", "count", "debug", "error", "group", "groupCollapsed", 
        "groupEnd", "info", "log", "time", "timeEnd", "warn" 
    ];
    for (i = methods.length-1; i>=0; --i)
    {
        method = methods[i];
        o[method] = { value : execute.bind(null, method) };
    }
    Object.defineProperties(console, o);
    Object.freeze(console);
})();
