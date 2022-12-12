
#include <InternalLoader.hpp>
#include <array>
#include <Geode/modify/LoadingLayer.hpp>
#include <fmt/format.h>
#include <Geode/utils/cocos.hpp>

USE_GEODE_NAMESPACE();

struct CustomLoadingLayer : Modify<CustomLoadingLayer, LoadingLayer> {
    bool m_updatingResources;

    CustomLoadingLayer() : m_updatingResources(false) {}

    bool init(bool fromReload) {
        Loader::get()->waitForModsToBeLoaded();

        if (!LoadingLayer::init(fromReload)) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        auto count = Loader::get()->getAllMods().size();

        auto label = CCLabelBMFont::create(
            fmt::format("Geode: Loaded {} mods", count).c_str(),
            "goldFont.fnt"
        );
        label->setPosition(winSize.width / 2, 30.f);
        label->setScale(.45f);
        label->setID("geode-loaded-info");
        this->addChild(label);

        // for some reason storing the listener as a field caused the 
        // destructor for the field not to be run
        this->addChild(EventListenerNode<ResourceDownloadFilter>::create(
            this, &CustomLoadingLayer::updateResourcesProgress
        ));

        // verify loader resources
        if (!InternalLoader::get()->verifyLoaderResources()) {
            m_fields->m_updatingResources = true;
            this->setUpdateText("Downloading Resources");
        }

        return true;
    }

    void setUpdateText(std::string const& text) {
        m_textArea->setString(text.c_str());
    }

    void updateResourcesProgress(ResourceDownloadEvent* event) {
        std::visit(makeVisitor {
            [&](UpdateProgress const& progress) {
                this->setUpdateText(fmt::format(
                    "Downloading Resources: {}%", progress.first
                ));
            },
            [&](UpdateFinished) {
                this->setUpdateText("Resources Downloaded");
                m_fields->m_updatingResources = false;
                this->loadAssets();
            },
            [&](UpdateFailed const& error) {
                InternalLoader::platformMessageBox(
                    "Error updating resources",
                    "Unable to update Geode resources: " + 
                    error + ".\n"
                    "The game will be loaded as normal, but please be aware "
                    "that it may very likely crash."
                );
                this->setUpdateText("Resource Download Failed");
                m_fields->m_updatingResources = false;
                this->loadAssets();
            }
        }, event->status);
    }

    void loadAssets() {
        if (m_fields->m_updatingResources) {
            return;
        }
        LoadingLayer::loadAssets();
    }
};
