// First-run wizard in the Nano skin (ui/NanoScreens.cpp paintWizard). The
// step order and the open*/select* functions in App.cpp stay as they were;
// this file only draws the wizard's own screens and routes their taps.
// Wi-Fi, the font picker and the library are the regular screens, reused
// with the wizard flags. Included at the end of App.cpp.

namespace {

constexpr int kWizardNext = 1;
constexpr int kWizardBack = 2;
constexpr int kWizardExtra = 3;
constexpr int kWizardChipBase = 100;
// Language, theme, color, Wi-Fi, loading, font, reading mode, app download,
// pairing, library.
constexpr size_t kWizardStepCount = 10;
// Reading theme shown by each Motyw chip (wizard order Light, Dark, Night;
// DisplayManager themes 0 dark, 1 light, 2 night).
constexpr uint8_t kWizardThemeChip[] = {1, 0, 2};

}  // namespace

bool App::wizardNanoScreen() const {
  switch (menuScreen_) {
    case MenuScreen::WelcomeLanguage:
    case MenuScreen::WelcomeSdCard:
    case MenuScreen::WelcomeTheme:
    case MenuScreen::WelcomeHighlightColor:
    case MenuScreen::WelcomeLoading:
    case MenuScreen::WelcomeSuper:
    case MenuScreen::WelcomeConfigureIntro:
    case MenuScreen::WelcomeReadingMode:
    case MenuScreen::WelcomeConnect:
    case MenuScreen::WelcomeAppPairing:
    case MenuScreen::WelcomeConfigureInApp:
      return true;
    default:
      return false;
  }
}

size_t App::wizardStepIndex() const {
  switch (menuScreen_) {
    case MenuScreen::WelcomeLanguage:
      return 0;
    case MenuScreen::WelcomeSdCard:
    case MenuScreen::WelcomeTheme:
      return 1;
    case MenuScreen::WelcomeHighlightColor:
      return 2;
    case MenuScreen::WelcomeLoading:
    case MenuScreen::WelcomeSuper:
    case MenuScreen::WelcomeConfigureIntro:
      return 4;
    case MenuScreen::WelcomeReadingMode:
      return 6;
    case MenuScreen::WelcomeConnect:
      return 7;
    case MenuScreen::WelcomeAppPairing:
      return 8;
    case MenuScreen::WelcomeConfigureInApp:
    default:
      return 9;
  }
}

void App::renderWizardPage() {
  const uint32_t nowMs = millis();
  const uint32_t elapsed = nowMs - welcomeScreenEnteredMs_;
  nano::WizardView view;
  view.step = wizardStepIndex();
  view.stepCount = kWizardStepCount;
  view.backId = kWizardBack;
  view.backLabel = uiText(UiText::Back);
  view.nextId = kWizardNext;
  view.nextLabel = tr3(TrKey3::NextLabel);

  auto addChips = [&](nano::WizardChipArt art) {
    for (size_t i = 0; i < settingsMenuItems_.size(); ++i) {
      nano::WizardChip chip;
      chip.id = kWizardChipBase + static_cast<int>(i);
      chip.label = settingsMenuItems_[i];
      chip.selected = i == settingsSelectedIndex_;
      chip.art = art;
      view.chips.push_back(chip);
    }
  };

  switch (menuScreen_) {
    case MenuScreen::WelcomeLanguage:
      view.title = tr3(TrKey3::WelcomeLanguageTitle);
      view.subtitle = tr4(TrKey4::WizLanguageSub);
      view.backId = nano::kNoTarget;
      view.footer = tr3(TrKey3::WelcomePowerBackHint);
      addChips(nano::WizardChipArt::Label);
      break;
    case MenuScreen::WelcomeTheme:
      view.title = tr3(TrKey3::WelcomeThemeTitle);
      view.subtitle = tr4(TrKey4::WizThemeSub);
      addChips(nano::WizardChipArt::Theme);
      for (size_t i = 0; i < view.chips.size() && i < 3; ++i) {
        view.chips[i].theme = kWizardThemeChip[i];
      }
      break;
    case MenuScreen::WelcomeHighlightColor:
      view.title = tr3(TrKey3::WelcomeHighlightColorTitle);
      view.subtitle = tr4(TrKey4::WizColorSub);
      addChips(nano::WizardChipArt::Swatch);
      for (size_t i = 0; i < view.chips.size(); ++i) {
        view.chips[i].swatch = DisplayManager::presetFocusColor(static_cast<uint8_t>(i));
      }
      break;
    case MenuScreen::WelcomeSdCard:
      view.body = nano::WizardBody::Message;
      view.extraId = kWizardExtra;
      view.extraLabel = tr4(TrKey4::WizSdSkip);
      switch (welcomeSdState_) {
        case WelcomeSdState::Missing:
          view.title = tr4(TrKey4::WizSdMissingTitle);
          view.subtitle = tr4(TrKey4::WizSdMissingSub);
          view.nextLabel = tr4(TrKey4::WizSdCheck);
          break;
        case WelcomeSdState::Unreadable:
          view.title = tr4(TrKey4::WizSdFormatTitle);
          view.subtitle = tr4(TrKey4::WizSdFormatSub);
          view.nextLabel = tr4(TrKey4::WizSdFormat);
          break;
        case WelcomeSdState::ConfirmFormat:
          view.title = tr4(TrKey4::WizSdFormatTitle);
          view.subtitle = tr4(TrKey4::WizSdConfirmSub);
          view.nextLabel = tr4(TrKey4::WizSdFormatYes);
          break;
        case WelcomeSdState::Formatting:
          view.body = nano::WizardBody::Loading;
          view.title = tr4(TrKey4::WizSdFormatting);
          view.subtitle = tr4(TrKey4::WizSdFormattingSub);
          view.backId = nano::kNoTarget;
          view.nextId = nano::kNoTarget;
          view.extraId = nano::kNoTarget;
          break;
        case WelcomeSdState::Failed:
          view.title = tr4(TrKey4::WizSdFailedTitle);
          view.subtitle = tr4(TrKey4::WizSdFailedSub);
          view.nextLabel = tr4(TrKey4::WizSdFormat);
          break;
      }
      break;
    case MenuScreen::WelcomeReadingMode:
      view.title = tr3(TrKey3::WelcomeReadingModeTitle);
      view.subtitle = tr4(TrKey4::WizModeSub);
      addChips(nano::WizardChipArt::Rsvp);
      for (nano::WizardChip &chip : view.chips) {
        chip.word = tr4(TrKey4::TutWord);
      }
      if (view.chips.size() > 1) {
        view.chips[1].art = nano::WizardChipArt::Scroll;
      }
      view.extraId = kWizardExtra;
      view.extraLabel = tr4(TrKey4::WizPreview);
      break;
    case MenuScreen::WelcomeLoading: {
      view.body = nano::WizardBody::Loading;
      const size_t phrase = static_cast<size_t>((elapsed / kWelcomeLoadingPhraseCycleMs) % 3UL);
      view.title = phrase == 0   ? tr3(TrKey3::WelcomeLoadingPhrase1)
                   : phrase == 1 ? tr3(TrKey3::WelcomeLoadingPhrase2)
                                 : tr3(TrKey3::WelcomeLoadingPhrase3);
      view.subtitle = fontDownloadInProgress_ ? tr3(TrKey3::WelcomeLoadingBottomDownloading)
                                              : tr3(TrKey3::WelcomeLoadingBottomLoading);
      view.phase = elapsed / 50UL;
      view.backId = nano::kNoTarget;
      view.nextId = nano::kNoTarget;
      break;
    }
    case MenuScreen::WelcomeSuper:
      view.body = nano::WizardBody::Message;
      view.title = tr3(TrKey3::WelcomeSuperTitle);
      view.subtitle = tr4(TrKey4::WizSuperSub);
      view.autoPercent = static_cast<int>(std::min<uint32_t>(100, elapsed * 100UL / kWelcomeTimedMessageMs));
      break;
    case MenuScreen::WelcomeConfigureIntro:
      view.body = nano::WizardBody::Message;
      view.title = tr3(TrKey3::WelcomeConfigureTitle);
      view.subtitle = tr4(TrKey4::WizConfigureSub);
      view.autoPercent = static_cast<int>(std::min<uint32_t>(100, elapsed * 100UL / kWelcomeTimedMessageMs));
      break;
    case MenuScreen::WelcomeConfigureInApp:
      view.body = nano::WizardBody::Message;
      view.title = tr3(TrKey3::WelcomeConfigureInAppLine1);
      view.subtitle = tr3(TrKey3::WelcomeConfigureInAppLine2);
      view.autoPercent = static_cast<int>(std::min<uint32_t>(100, elapsed * 100UL / kWelcomeTimedMessageMs));
      break;
    case MenuScreen::WelcomeConnect: {
      ensureInstallAppQr();
      view.body = nano::WizardBody::Qr;
      view.title = tr3(TrKey3::WelcomeConnectTitle);
      view.subtitle = tr3(TrKey3::WelcomeConnectLine1);
      view.qr = g_installAppQrSize > 0 ? g_installAppQrData : nullptr;
      view.qrSize = g_installAppQrSize;
      view.qrLine = "flower.theworkpc.com/appdownload";
      // Dalej only after a few seconds with the code on screen, as before.
      const bool showNext = g_installAppQrSize == 0 || elapsed >= kWelcomeConnectNextDelayMs;
      if (!showNext) {
        view.nextId = nano::kNoTarget;
        view.qrHint = tr4(TrKey4::WizConnectLook);
      }
      break;
    }
    case MenuScreen::WelcomeAppPairing:
      view.body = nano::WizardBody::Qr;
      view.title = tr3(TrKey3::WelcomeAppPairingTitle);
      view.subtitle = tr4(TrKey4::WizPairSub);
      if (companionSync_.hasQrCode()) {
        view.qr = companionSync_.qrCodeData();
        view.qrSize = companionSync_.qrCodeSize();
      }
      view.qrLine = companionSync_.statusLine1();
      view.qrHint = autoSyncClientConnected_ ? tr3(TrKey3::WelcomeConnectHintConnected)
                                             : tr3(TrKey3::WelcomeConnectHintWaiting);
      break;
    default:
      return;
  }

  const bool classicColors = navMode_ != NavMode::Modern;
  if (classicColors) {
    display_.overrideNanoPalette(DisplayManager::kNanoPaletteClassic, false);
  }
  wizardTargets_.clear();
  NanoPanelSink sink(wizardTargets_, -1);
  nano::paintWizard(display_, sink, view);
  if (classicColors) {
    display_.overrideNanoPalette(nanoPalette_, nanoOwnAccent_);
  }
}

void App::handleWizardTouchAt(uint16_t x, uint16_t y, uint32_t nowMs) {
  int hit = nano::kNoTarget;
  for (const auto &target : wizardTargets_) {
    const ui::Rect &r = target.first;
    if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) {
      hit = target.second;
      break;
    }
  }
  if (hit == nano::kNoTarget) {
    return;
  }
  if (hit >= kWizardChipBase) {
    settingsSelectedIndex_ = static_cast<size_t>(hit - kWizardChipBase);
    previewWizardPickerSelection(nowMs);
    renderWizardPage();
    return;
  }
  if (hit == kWizardBack) {
    wizardStepBack(nowMs);
    return;
  }
  if (hit == kWizardExtra) {
    if (menuScreen_ == MenuScreen::WelcomeReadingMode) {
      openWelcomeReadingModePreview(static_cast<uint8_t>(settingsSelectedIndex_ == 1 ? 1 : 0));
    } else if (menuScreen_ == MenuScreen::WelcomeSdCard) {
      openWelcomeTheme();
    }
    return;
  }
  switch (menuScreen_) {
    case MenuScreen::WelcomeLanguage:
      selectWelcomeLanguageItem(nowMs);
      return;
    case MenuScreen::WelcomeSdCard:
      selectWelcomeSdCardNext(nowMs);
      return;
    case MenuScreen::WelcomeTheme:
      selectWelcomeThemeItem(nowMs);
      return;
    case MenuScreen::WelcomeHighlightColor:
      selectWelcomeHighlightColorItem(nowMs);
      return;
    case MenuScreen::WelcomeReadingMode:
      selectWelcomeReadingModeItem(nowMs);
      return;
    case MenuScreen::WelcomeSuper:
      openWelcomeConfigureIntro(nowMs);
      return;
    case MenuScreen::WelcomeConfigureIntro:
      wizardFontPickerActive_ = true;
      openTypographyFontPicker();
      return;
    case MenuScreen::WelcomeConnect:
      selectWelcomeConnectTap(nowMs);
      return;
    case MenuScreen::WelcomeAppPairing:
      selectWelcomeAppPairingTap(nowMs);
      return;
    case MenuScreen::WelcomeConfigureInApp:
      openWelcomeBookPicker(nowMs);
      return;
    default:
      return;
  }
}
