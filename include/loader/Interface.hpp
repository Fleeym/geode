#pragma once

#include "Log.hpp"
#include "Mod.hpp"
#include "Types.hpp"
#include "../Macros.hpp"
#include "../utils/casts.hpp"
#include "../utils/Result.hpp"
#include <vector>
#include <variant>

namespace geode {
	class Hook;
	class LogPtr;

	/**
	 * For developing your own mods, it is 
	 * often convenient to be able to do things 
	 * like create hooks using statically 
	 * initialized global classes.
	 * 
	 * At that point however, your mod has not 
	 * yet received the Mod* to create hooks 
	 * through.
	 * 
	 * For situations like that, you can instead 
	 * inherit from Interface to create your own 
	 * mod interface and create hooks through that; 
	 * calling `init` with the Mod* you receive 
	 * from geode will automatically create all 
	 * scheduled hooks, logs, etc.
	 * 
	 * Interface also provides a handy & 
	 * standardized way to store access to your 
	 * Mod*; you can just define a `get` function 
	 * and a getter for the Mod* stored in the 
	 * Interface.
	 * 
	 * @class Interface
	 */
	class Interface {
	protected:
		static GEODE_DLL Interface* create();
		

		struct ScheduledHook {
			std::string m_displayName;
			void* m_address;
			Result<Hook*>(Mod::*m_addFunction)(std::string const&, void*);
		};

		struct ScheduledLog {
			std::string m_info;
			Severity m_severity;
		};

		using exportmemfn_t = void(Mod::*)(std::string const&, unknownmemfn_t);
		using exportfn_t = void(Mod::*)(std::string const&, unknownfn_t);

		using loadfn_t = void(*)(Mod*);

		struct ScheduledExport {
			std::string m_selector;
			std::variant<unknownmemfn_t, unknownfn_t> m_func;
		};

		Mod* m_mod = nullptr;
		std::vector<ScheduledHook> m_scheduledHooks;
		std::vector<ScheduledLog> m_scheduledLogs;
		std::vector<ScheduledExport> m_scheduledExports;
		std::vector<loadfn_t> m_scheduledFunctions;
		
	public:
		static inline GEODE_HIDDEN Interface* get() {
			static Interface* ret = create();
			return ret;
		}

		[[deprecated("Use Mod::get instead")]]
		static inline GEODE_HIDDEN Mod* mod() {
			return Interface::get()->m_mod;
		}

		[[deprecated("Use Log::get instead")]]
		static inline GEODE_HIDDEN Log log() {
			return Interface::get()->m_mod->log();
		}

		GEODE_DLL void init(Mod*);

        /**
         * Create a hook at an address. This function can 
		 * be used at static initialization time, as it 
		 * doesn't require the Mod* to be set -- it will 
		 * create the hooks later when the Mod* is set.
		 * 
         * @param address The absolute address of 
         * the function to hook, i.e. gd_base + 0xXXXX
         * @param detour Pointer to your detour function
		 * 
         * @returns Successful result containing the 
         * Hook handle (or nullptr if Mod* is not loaded 
		 * yet), errorful result with info on error
         */
        template<auto Detour, template <class, class...> class Convention>
        Result<Hook*> addHook(void* address) {
        	return Interface::addHook<Detour, Convention>("", address);
        }

        /**
         * The same as addHook(void*, void*), but also provides 
         * a display name to show it in the list of the loader.
         * Mostly for internal use but if you don't like your
         * hooks showing up like base + 0x123456 it can be useful
         */
        template<auto Detour, template <class, class...> class Convention>
        Result<Hook*> addHook(std::string const& displayName, void* address) {
        	if (this->m_mod) {
        		return this->m_mod->addHook<Detour, Convention>(displayName, address);
        	}
        	this->m_scheduledHooks.push_back({ displayName, address, &Mod::addHook<Detour, Convention> });
			return Ok<Hook*>(nullptr);
        }

        /**
         * Log an information. Equivalent to 
         * ```
         * Mod::log() << Severity::severity << info.
         * ```
         * @param info Log information
         * @param severity Log severity
         */
        GEODE_DLL void logInfo(std::string const& info, Severity severity);

        GEODE_DLL void scheduleOnLoad(loadfn_t fn);

    protected:
        GEODE_DLL void exportAPIFunctionInternal(std::string const& selector, unknownmemfn_t fn);
        GEODE_DLL void exportAPIFunctionInternal(std::string const& selector, unknownfn_t fn);

    public:
        template <typename T>
        inline void exportAPIFunction(std::string const& selector, T ptr) {
        	if constexpr (std::is_member_function_pointer_v<decltype(ptr)>) {
        		exportAPIFunctionInternal(selector, cast::reference_cast<unknownmemfn_t>(ptr));
        	}
        	else {
        		exportAPIFunctionInternal(selector, reinterpret_cast<unknownfn_t>(ptr));
        	}
        }

        friend Mod* Mod::get<void>();
	};

	template<>
	inline Mod* Mod::get<void>() {
		return Interface::get()->m_mod;
	}

	inline Log Log::get() {
		return Mod::get()->log();
	}

	template <typename T>
	inline Observer<std::monostate>* EventCenter::registerObserver(std::string sel, std::function<void(Event<T> const&)> cb) {
	    return registerObserver(Mod::get(), EventInfo<T>(sel), cb);
	}

	template <typename T>
	inline Observer<std::monostate>* EventCenter::registerObserver(EventInfo<T> info, std::function<void(Event<T> const&)> cb) {
	    return registerObserver(Mod::get(), info, cb);
	}
    
}

inline const char* operator"" _spr(const char* str, size_t) {
    return geode::Mod::get()->expandSpriteName(str);
}
