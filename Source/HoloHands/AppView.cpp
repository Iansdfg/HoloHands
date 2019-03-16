#include "pch.h"

#include "AppMain.h"
#include "AppView.h"

[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
    HoloHands::AppViewSource^ appViewSource = ref new HoloHands::AppViewSource();

    Windows::ApplicationModel::Core::CoreApplication::Run(appViewSource);

    return 0;
}

namespace HoloHands
{
    Windows::ApplicationModel::Core::IFrameworkView^ AppViewSource::CreateView()
    {
        return ref new AppView();
    }

    AppView::AppView()
    {
    }

    std::shared_ptr<Holographic::AppMainBase> AppView::InitializeCore()
    {
        return std::make_shared<AppMain>(_deviceResources);
    }
}
