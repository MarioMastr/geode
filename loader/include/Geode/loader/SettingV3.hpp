#pragma once

#include "../DefaultInclude.hpp"
#include <optional>
#include <cocos2d.h>
// todo: remove this header in 4.0.0
#include "Setting.hpp"
#include "../utils/cocos.hpp"
// this unfortunately has to be included because of C++ templates
#include "../utils/JsonValidation.hpp"
#include "../utils/function.hpp"

// todo in v4: this can be removed as well as the friend decl in LegacyCustomSettingV3
class LegacyCustomSettingToV3Node;
class ModSettingsPopup;

namespace geode {
    class ModSettingsManager;
    class SettingNodeV3;
    // todo in v4: remove this
    class SettingValue;

    class GEODE_DLL SettingV3 : public std::enable_shared_from_this<SettingV3> {
    private:
        class GeodeImpl;
        std::shared_ptr<GeodeImpl> m_impl;
    
    protected:
        void init(std::string const& key, std::string const& modID);
        Result<> parseSharedProperties(std::string const& key, std::string const& modID, matjson::Value const& value, bool onlyNameAndDesc = false);
        void parseSharedProperties(std::string const& key, std::string const& modID, JsonExpectedValue& value, bool onlyNameAndDesc = false);

        /**
         * Mark that the value of this setting has changed. This should be 
         * ALWAYS called on every setter that can modify the setting's state!
         */
        void markChanged();

        friend class ::geode::SettingValue;

    public:
        SettingV3();
        virtual ~SettingV3();

        /**
         * Get the key of this setting
         */
        std::string getKey() const;
        /**
         * Get the mod ID this setting is for
         */
        std::string getModID() const;
        /**
         * Get the mod this setting is for. Note that this may return null 
         * while the mod is still being initialized
         */
        Mod* getMod() const;
        /**
         * Get the name of this setting
         */
        std::optional<std::string> getName() const; 
        /**
         * Get the name of this setting, or its key if it has no name
         */
        std::string getDisplayName() const; 
        /**
         * Get the description of this setting
         */
        std::optional<std::string> getDescription() const;
        /**
         * Get the "enable-if" scheme for this setting
         */
        std::optional<std::string> getEnableIf() const;
        /**
         * Check if this setting should be enabled based on the "enable-if" scheme
         */
        bool shouldEnable() const;
        std::optional<std::string> getEnableIfDescription() const;
        /**
         * Whether this setting requires a restart on change
         */
        bool requiresRestart() const;

        virtual bool load(matjson::Value const& json) = 0;
        virtual bool save(matjson::Value& json) const = 0;
        virtual SettingNodeV3* createNode(float width) = 0;

        virtual bool isDefaultValue() const = 0;
        /**
         * Reset this setting's value back to its original value
         */
        virtual void reset() = 0;

        [[deprecated("This function will be removed alongside legacy settings in 4.0.0!")]]
        virtual std::optional<Setting> convertToLegacy() const;
        [[deprecated("This function will be removed alongside legacy settings in 4.0.0!")]]
        virtual std::optional<std::shared_ptr<SettingValue>> convertToLegacyValue() const;
    };
    
    using SettingGenerator = std::function<Result<std::shared_ptr<SettingV3>>(
        std::string const& key,
        std::string const& modID,
        matjson::Value const& json
    )>;

    namespace detail {
        template <class T, class V = T>
        class GeodeSettingBaseValueV3 : public SettingV3 {
        protected:
            virtual T& getValueMut() const = 0;

            template <class D>
            void parseDefaultValue(JsonExpectedValue& json, D& defaultValue) {
                auto value = json.needs("default");
                // Check if this is a platform-specific default value
                if (value.isObject() && value.has(GEODE_PLATFORM_SHORT_IDENTIFIER_NOARCH)) {
                    value.needs(GEODE_PLATFORM_SHORT_IDENTIFIER_NOARCH).into(defaultValue);
                }
                else {
                    value.into(defaultValue);
                }
            }

        public:
            using ValueType = T;

            virtual T getDefaultValue() const = 0;

            T getValue() const {
                return this->getValueMut();
            }
            void setValue(V value) {
                this->getValueMut() = this->isValid(value) ? value : this->getDefaultValue();
                this->markChanged();
            }
            virtual Result<> isValid(V value) const = 0;
            
            bool isDefaultValue() const override {
                return this->getValue() == this->getDefaultValue();
            }
            void reset() override {
                this->setValue(this->getDefaultValue());
            }
        };
    }

    class GEODE_DLL TitleSettingV3 final : public SettingV3 {
    private:
        class Impl;
        std::shared_ptr<Impl> m_impl;
    
    private:
        class PrivateMarker {};
        friend class SettingV3;

    public:
        TitleSettingV3(PrivateMarker);
        static Result<std::shared_ptr<TitleSettingV3>> parse(std::string const& key, std::string const& modID, matjson::Value const& json);

        bool load(matjson::Value const& json) override;
        bool save(matjson::Value& json) const override;
        SettingNodeV3* createNode(float width) override;

        bool isDefaultValue() const override;
        void reset() override;
    };

    // todo in v4: remove this class completely
    class GEODE_DLL LegacyCustomSettingV3 final : public SettingV3 {
    private:
        class Impl;
        std::shared_ptr<Impl> m_impl;

        friend class ::geode::ModSettingsManager;
        friend class ::LegacyCustomSettingToV3Node;
    
    private:
        class PrivateMarker {};
        friend class SettingV3;

    public:
        LegacyCustomSettingV3(PrivateMarker);
        static Result<std::shared_ptr<LegacyCustomSettingV3>> parse(std::string const& key, std::string const& modID, matjson::Value const& json);

        std::shared_ptr<SettingValue> getValue() const;
        void setValue(std::shared_ptr<SettingValue> value);

        bool load(matjson::Value const& json) override;
        bool save(matjson::Value& json) const override;
        SettingNodeV3* createNode(float width) override;

        bool isDefaultValue() const override;
        void reset() override;
        
        std::optional<Setting> convertToLegacy() const override;
        std::optional<std::shared_ptr<SettingValue>> convertToLegacyValue() const override;
    };

    class GEODE_DLL BoolSettingV3 final : public detail::GeodeSettingBaseValueV3<bool> {
    private:
        class Impl;
        std::shared_ptr<Impl> m_impl;

    protected:
        bool& getValueMut() const override;

    private:
        class PrivateMarker {};
        friend class SettingV3;

    public:
        BoolSettingV3(PrivateMarker);
        static Result<std::shared_ptr<BoolSettingV3>> parse(std::string const& key, std::string const& modID, matjson::Value const& json);

        bool getDefaultValue() const override;
        Result<> isValid(bool value) const override;
        
        bool load(matjson::Value const& json) override;
        bool save(matjson::Value& json) const override;
        SettingNodeV3* createNode(float width) override;

        std::optional<Setting> convertToLegacy() const override;
        std::optional<std::shared_ptr<SettingValue>> convertToLegacyValue() const override;
    };

    class GEODE_DLL IntSettingV3 final : public detail::GeodeSettingBaseValueV3<int64_t> {
    private:
        class Impl;
        std::shared_ptr<Impl> m_impl;

    protected:
        int64_t& getValueMut() const override;

    private:
        class PrivateMarker {};
        friend class SettingV3;

    public:
        IntSettingV3(PrivateMarker);
        static Result<std::shared_ptr<IntSettingV3>> parse(std::string const& key, std::string const& modID, matjson::Value const& json);

        int64_t getDefaultValue() const override;
        Result<> isValid(int64_t value) const override;

        std::optional<int64_t> getMinValue() const;
        std::optional<int64_t> getMaxValue() const;

        bool isArrowsEnabled() const;
        bool isBigArrowsEnabled() const;
        size_t getArrowStepSize() const;
        size_t getBigArrowStepSize() const;
        bool isSliderEnabled() const;
        std::optional<int64_t> getSliderSnap() const;
        bool isInputEnabled() const;
    
        bool load(matjson::Value const& json) override;
        bool save(matjson::Value& json) const override;
        SettingNodeV3* createNode(float width) override;

        std::optional<Setting> convertToLegacy() const override;
        std::optional<std::shared_ptr<SettingValue>> convertToLegacyValue() const override;
    };

    class GEODE_DLL FloatSettingV3 final : public detail::GeodeSettingBaseValueV3<double> {
    private:
        class Impl;
        std::shared_ptr<Impl> m_impl;

    protected:
        double& getValueMut() const override;

    private:
        class PrivateMarker {};
        friend class SettingV3;

    public:
        FloatSettingV3(PrivateMarker);
        static Result<std::shared_ptr<FloatSettingV3>> parse(std::string const& key, std::string const& modID, matjson::Value const& json);

        double getDefaultValue() const override;
        Result<> isValid(double value) const override;

        std::optional<double> getMinValue() const;
        std::optional<double> getMaxValue() const;

        bool isArrowsEnabled() const;
        bool isBigArrowsEnabled() const;
        double getArrowStepSize() const;
        double getBigArrowStepSize() const;
        bool isSliderEnabled() const;
        std::optional<double> getSliderSnap() const;
        bool isInputEnabled() const;
        
        bool load(matjson::Value const& json) override;
        bool save(matjson::Value& json) const override;
        SettingNodeV3* createNode(float width) override;

        std::optional<Setting> convertToLegacy() const override;
        std::optional<std::shared_ptr<SettingValue>> convertToLegacyValue() const override;
    };

    class GEODE_DLL StringSettingV3 final : public detail::GeodeSettingBaseValueV3<std::string, std::string_view> {
    private:
        class Impl;
        std::shared_ptr<Impl> m_impl;

    protected:
        std::string& getValueMut() const override;

    private:
        class PrivateMarker {};
        friend class SettingV3;

    public:
        StringSettingV3(PrivateMarker);
        static Result<std::shared_ptr<StringSettingV3>> parse(std::string const& key, std::string const& modID, matjson::Value const& json);

        std::string getDefaultValue() const override;
        Result<> isValid(std::string_view value) const override;

        std::optional<std::string> getRegexValidator() const;
        std::optional<std::string> getAllowedCharacters() const;
        std::optional<std::vector<std::string>> getEnumOptions() const;
        
        bool load(matjson::Value const& json) override;
        bool save(matjson::Value& json) const override;
        SettingNodeV3* createNode(float width) override;

        std::optional<Setting> convertToLegacy() const override;
        std::optional<std::shared_ptr<SettingValue>> convertToLegacyValue() const override;
    };

    class GEODE_DLL FileSettingV3 final : public detail::GeodeSettingBaseValueV3<std::filesystem::path, std::filesystem::path const&> {
    private:
        class Impl;
        std::shared_ptr<Impl> m_impl;

    protected:
        std::filesystem::path& getValueMut() const override;

    private:
        class PrivateMarker {};
        friend class SettingV3;

    public:
        FileSettingV3(PrivateMarker);
        static Result<std::shared_ptr<FileSettingV3>> parse(std::string const& key, std::string const& modID, matjson::Value const& json);

        std::filesystem::path getDefaultValue() const override;
        Result<> isValid(std::filesystem::path const& value) const override;

        bool isFolder() const;
        bool useSaveDialog() const;

        std::optional<std::vector<utils::file::FilePickOptions::Filter>> getFilters() const;
        
        bool load(matjson::Value const& json) override;
        bool save(matjson::Value& json) const override;
        SettingNodeV3* createNode(float width) override;

        std::optional<Setting> convertToLegacy() const override;
        std::optional<std::shared_ptr<SettingValue>> convertToLegacyValue() const override;
    };

    class GEODE_DLL Color3BSettingV3 final : public detail::GeodeSettingBaseValueV3<cocos2d::ccColor3B> {
    private:
        class Impl;
        std::shared_ptr<Impl> m_impl;

    protected:
        cocos2d::ccColor3B& getValueMut() const override;

    private:
        class PrivateMarker {};
        friend class SettingV3;

    public:
        Color3BSettingV3(PrivateMarker);
        static Result<std::shared_ptr<Color3BSettingV3>> parse(std::string const& key, std::string const& modID, matjson::Value const& json);

        cocos2d::ccColor3B getDefaultValue() const override;
        Result<> isValid(cocos2d::ccColor3B value) const override;

        bool load(matjson::Value const& json) override;
        bool save(matjson::Value& json) const override;
        SettingNodeV3* createNode(float width) override;

        std::optional<Setting> convertToLegacy() const override;
        std::optional<std::shared_ptr<SettingValue>> convertToLegacyValue() const override;
    };

    class GEODE_DLL Color4BSettingV3 final : public detail::GeodeSettingBaseValueV3<cocos2d::ccColor4B> {
    private:
        class Impl;
        std::shared_ptr<Impl> m_impl;

    protected:
        cocos2d::ccColor4B& getValueMut() const override;

    private:
        class PrivateMarker {};
        friend class SettingV3;

    public:
        Color4BSettingV3(PrivateMarker);
        static Result<std::shared_ptr<Color4BSettingV3>> parse(std::string const& key, std::string const& modID, matjson::Value const& json);

        cocos2d::ccColor4B getDefaultValue() const override;
        Result<> isValid(cocos2d::ccColor4B value) const override;

        bool load(matjson::Value const& json) override;
        bool save(matjson::Value& json) const override;
        SettingNodeV3* createNode(float width) override;

        std::optional<Setting> convertToLegacy() const override;
        std::optional<std::shared_ptr<SettingValue>> convertToLegacyValue() const override;
    };

    class GEODE_DLL SettingNodeV3 : public cocos2d::CCNode {
    private:
        class Impl;
        std::shared_ptr<Impl> m_impl;
        
        friend class ::ModSettingsPopup;

    protected:
        bool init(std::shared_ptr<SettingV3> setting, float width);

        virtual void updateState();

        /**
         * Mark this setting as changed. This updates the UI for committing 
         * the value
         */
        void markChanged();

        /**
         * When the setting value is committed (aka can't be undone), this 
         * function will be called. This should take care of actually saving 
         * the value in some sort of global manager
         */
        virtual void onCommit() = 0;
        virtual void onResetToDefault() = 0;

        void onDescription(CCObject*);
        void onReset(CCObject*);

    public:
        void commit();
        void resetToDefault();
        virtual bool hasUncommittedChanges() const = 0;
        virtual bool hasNonDefaultValue() const = 0;

        // Can be overridden by the setting itself
        // Can / should be used to do alternating BG
        void setBGColor(cocos2d::ccColor4B const& color);

        cocos2d::CCLabelBMFont* getNameLabel() const;
        cocos2d::CCMenu* getNameMenu() const;
        cocos2d::CCMenu* getButtonMenu() const;

        void setContentSize(cocos2d::CCSize const& size) override;

        std::shared_ptr<SettingV3> getSetting() const;
    };

    class GEODE_DLL SettingChangedEventV3 final : public Event {
    private:
        class Impl;
        std::shared_ptr<Impl> m_impl;
    
    public:
        SettingChangedEventV3(std::shared_ptr<SettingV3> setting);

        std::shared_ptr<SettingV3> getSetting() const;
    };
    class GEODE_DLL SettingChangedFilterV3 final : public EventFilter<SettingChangedEventV3> {
    private:
        class Impl;
        std::shared_ptr<Impl> m_impl;
    
    public:
        using Callback = void(std::shared_ptr<SettingV3>);

        ListenerResult handle(utils::MiniFunction<Callback> fn, SettingChangedEventV3* event);
        /**
         * Listen to changes on a setting, or all settings
         * @param modID Mod whose settings to listen to
         * @param settingKey Setting to listen to, or all settings if nullopt
         */
        SettingChangedFilterV3(
            std::string const& modID,
            std::optional<std::string> const& settingKey
        );
        SettingChangedFilterV3(Mod* mod, std::optional<std::string> const& settingKey);
        SettingChangedFilterV3(SettingChangedFilterV3 const&);
    };

    class GEODE_DLL SettingNodeSizeChangeEventV3 : public Event {
    private:
        class Impl;
        std::shared_ptr<Impl> m_impl;
    
    public:
        SettingNodeSizeChangeEventV3(SettingNodeV3* node);
        virtual ~SettingNodeSizeChangeEventV3();

        SettingNodeV3* getNode() const;
    };
    class GEODE_DLL SettingNodeValueChangeEventV3 : public Event {
    private:
        class Impl;
        std::shared_ptr<Impl> m_impl;
    
    public:
        SettingNodeValueChangeEventV3(SettingNodeV3* node, bool commit);
        virtual ~SettingNodeValueChangeEventV3();

        SettingNodeV3* getNode() const;
        bool isCommit() const;
    };

    template <class T>
    struct SettingTypeForValueType {
        static_assert(
            !std::is_same_v<T, T>,
            "specialize the SettingTypeForValueType class to use Mod::getSettingValue for custom settings"
        );
    };

    template <>
    struct SettingTypeForValueType<bool> {
        using SettingType = BoolSettingV3;
    };
    template <>
    struct SettingTypeForValueType<int64_t> {
        using SettingType = IntSettingV3;
    };
    template <>
    struct SettingTypeForValueType<double> {
        using SettingType = FloatSettingV3;
    };
    template <>
    struct SettingTypeForValueType<std::string> {
        using SettingType = StringSettingV3;
    };
    template <>
    struct SettingTypeForValueType<std::filesystem::path> {
        using SettingType = FileSettingV3;
    };
    template <>
    struct SettingTypeForValueType<cocos2d::ccColor3B> {
        using SettingType = Color3BSettingV3;
    };
    template <>
    struct SettingTypeForValueType<cocos2d::ccColor4B> {
        using SettingType = Color4BSettingV3;
    };

    template <class T>
    EventListener<SettingChangedFilterV3>* listenForSettingChanges(std::string_view settingKey, auto&& callback, Mod* mod = getMod()) {
        using Ty = typename SettingTypeForValueType<T>::SettingType;
        return new EventListener(
            [callback = std::move(callback)](std::shared_ptr<SettingV3> setting) {
                if (auto ty = typeinfo_pointer_cast<Ty>(setting)) {
                    callback(ty->getValue());
                }
            },
            SettingChangedFilterV3(mod, std::string(settingKey))
        );
    }
    EventListener<SettingChangedFilterV3>* listenForSettingChanges(std::string_view settingKey, auto&& callback, Mod* mod = getMod()) {
        using T = std::remove_cvref_t<utils::function::Arg<0, decltype(callback)>>;
        return listenForSettingChanges<T>(settingKey, std::move(callback), mod);
    }
    GEODE_DLL EventListener<SettingChangedFilterV3>* listenForAllSettingChanges(
        std::function<void(std::shared_ptr<SettingV3>)> const& callback,
        Mod* mod = getMod()
    );
}
