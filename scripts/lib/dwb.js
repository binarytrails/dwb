(function() {
    var _modules = {};
    var _requires = {};
    var _callbacks = [];
    var _initialized = false;
    var _applyRequired = function(names, callback) 
    {
        if (names === null) 
            callback.call(this, _modules);
        else 
        {
            var modules = [];
            var name, detail;
            for (var j=0, m=names.length; j<m; j++) 
            {
                name = names[j];
                if (/^\w*!/.test(name)) 
                {
                    detail = name.split("!");
                    name = detail[0];
                    if (!_modules[name]) 
                        include(detail.slice(1).join("!"));
                }
                modules.push(_modules[name]);
            } 
            if (callback)
                callback.apply(this, modules);
        }
    };
    var _contexts = {};
    var _setPrivate = function(id, object, key, value)
    {
        var realKey = key + id;
        if (object) 
        {
            if (object[realKey])
            {
                object[realKey] = value;
            }
            else 
            {
                Object.defineProperty(object, realKey, { value : value, writable : true });
            }
        }
    };
    var _getPrivate = function(id, object, key)
    {
        return object[key+id];
    };
    Object.defineProperties(this, { 
            "provide" : 
            { 
                value : function(name, module) 
                {
                    if (_modules[name]) 
                    {
                        io.debug({ 
                                offset : 1, arguments : arguments,
                                error : new Error("provide: Module " + 
                                                  name + " is already defined!")
                        });
                    }
                    else 
                        _modules[name] = module;
                }
            },
            "replace" : 
            {
                value : function(name, module) 
                {
                    if (! module && _modules[name] ) 
                    {
                        for (var key in _modules[name]) 
                            delete _modules[name][key];

                        delete _modules[name];
                    }
                    else 
                        _modules[name] = module;
                }
            },
            "require" : 
            {
                value : function(names, callback) 
                {
                    if (names !== null && ! (names instanceof Array)) 
                    {
                        io.debug({ 
                                error : new Error("require : invalid argument (" + 
                                                    JSON.stringify(names) + ")"), 
                                offset : 1, 
                                arguments : arguments 
                        });
                        return; 
                    }

                    if (!_initialized) 
                        _callbacks.push({callback : callback, names : names});
                    else 
                        _applyRequired(names, callback);
                }
            },
            "_initNewContext" : 
            {
                value : (function() {
                    var lastId = Math.ceil(Math.random() * 314159) + 271828;
                    var nextId = function() {
                            lastId++;
                            return String(Math.floor(Math.random() * 89999) + 10000) + lastId + 
                                String(Math.floor(Math.random() * 89999) + 10000); 
                    };
                    return function(self, arguments, path) {
                        var id = nextId();
                        _contexts[id] = self;
                        Object.defineProperties(self, { 
                                "path" : { value : path },
                                "debug" : { value : io.debug.bind(this) }, 
                                "_arguments" : { value : arguments },
                                "setPrivate" : { value : _setPrivate.bind(this, String(id)) }, 
                                "getPrivate" : { value : _getPrivate.bind(this, String(id)) }
                        });
                        Object.freeze(self);

                    };
                })() 
            },
            // Called after all scripts have been loaded and executed
            // Immediately deleted from the global object, so it is not callable
            // from other scripts
            "_initAfter" : 
            { 
                value : function() 
                {
                    var i;
                    _initialized = true;
                    for (i=0, l=_callbacks.length; i<l; i++) 
                        _applyRequired(_callbacks[i].names, _callbacks[i].callback);

                },
                configurable : true
            },
            "_initBefore" : 
            {
                value : function(contexts) 
                {
                    //_contexts = contexts;
                    Object.freeze(this);
                },
                configurable : true
            }
    });
    Object.defineProperties(GObject.prototype, {
            "notify" : 
            { 
                value : function(name, callback, after) 
                { 
                    return this.connect("notify::" + util.uncamelize(name), callback, after || false);
                }
            },
            "connectBlocked" : 
            { 
                value : function(name, callback, after) 
                { 
                    var sig = this.connect(name, (function() { 
                        this.blockSignal(sig);
                        var result = callback.apply(callback, arguments);
                        this.unblockSignal(sig);
                        return result;
                    }).bind(this));
                    return sig;
                }
            },
            "notifyBlocked" : 
            {
                value : function(name, callback, after) 
                {
                    return this.connectBlocked("notify::" + util.uncamelize(name), callback, after || false);
                }
            }
    });
    Object.defineProperties(Deferred.prototype, {
            "done" : {
                value : function(method) {
                    return this.then(method);
                }
            },
            "fail" : {
                value : function(method) {
                    return this.then(null, method);
                }
            },
            "always" : {
                value : function(method)
                {
                    return this.then(method, method);
                }
            }
    });
    Object.defineProperty(Deferred, "when", {
            value : function(value, callback, errback)
            {
                if (value instanceof Deferred)
                    return value.then(callback, errback);
                else 
                    return callback(value);
            }
    });
    if (!Function.prototype.debug) 
    {
        Object.defineProperty(Function.prototype, "debug", {
                value : function(scope)
                {
                    return function() {
                        try 
                        {
                            return this.apply(this, arguments);
                        }
                        catch (e) 
                        { 
                            if (scope)
                                scope.debug(e);
                            else 
                                io.debug(e);
                        }
                        return undefined;
                    }.bind(this);
                }
        });
    }
})();
Object.preventExtensions(this);
