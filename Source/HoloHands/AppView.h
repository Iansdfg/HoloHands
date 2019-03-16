#pragma once

namespace HoloHands
{
    ref class AppView sealed : public Holographic::AppViewBase
    {
    public:
        AppView();

    protected private:
        virtual std::shared_ptr<Holographic::AppMainBase> InitializeCore() override;
    };

    ref class AppViewSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
    {
    public:
        virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
    };
}
