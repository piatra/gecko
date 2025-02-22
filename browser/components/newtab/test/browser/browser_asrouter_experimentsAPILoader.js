const { RemoteSettings } = ChromeUtils.import(
  "resource://services-settings/remote-settings.js"
);
const { ASRouter } = ChromeUtils.import(
  "resource://activity-stream/lib/ASRouter.jsm"
);
const { RemoteSettingsExperimentLoader } = ChromeUtils.import(
  "resource://messaging-system/lib/RemoteSettingsExperimentLoader.jsm"
);
const { ExperimentAPI } = ChromeUtils.import(
  "resource://messaging-system/experiments/ExperimentAPI.jsm"
);
const { ExperimentFakes } = ChromeUtils.import(
  "resource://testing-common/MSTestUtils.jsm"
);
const { TelemetryFeed } = ChromeUtils.import(
  "resource://activity-stream/lib/TelemetryFeed.jsm"
);

const EXPERIMENT_PAYLOAD = ExperimentFakes.recipe("test_xman_cfr", {
  id: "xman_test_message",
  bucketConfig: {
    count: 100,
    start: 0,
    total: 100,
    namespace: "mochitest",
    randomizationUnit: "normandy_id",
  },
  branches: [
    {
      slug: "control",
      ratio: 1,
      feature: {
        featureId: "cfr",
        enabled: true,
        value: {
          id: "xman_test_message",
          content: {
            text: "This is a test CFR",
            addon: {
              id: "954390",
              icon:
                "chrome://activity-stream/content/data/content/assets/cfr_fb_container.png",
              title: "Facebook Container",
              users: 1455872,
              author: "Mozilla",
              rating: 4.5,
              amo_url:
                "https://addons.mozilla.org/firefox/addon/facebook-container/",
            },
            buttons: {
              primary: {
                label: {
                  string_id: "cfr-doorhanger-extension-ok-button",
                },
                action: {
                  data: {
                    url: null,
                  },
                  type: "INSTALL_ADDON_FROM_URL",
                },
              },
              secondary: [
                {
                  label: {
                    string_id: "cfr-doorhanger-extension-cancel-button",
                  },
                  action: {
                    type: "CANCEL",
                  },
                },
                {
                  label: {
                    string_id:
                      "cfr-doorhanger-extension-never-show-recommendation",
                  },
                },
                {
                  label: {
                    string_id:
                      "cfr-doorhanger-extension-manage-settings-button",
                  },
                  action: {
                    data: {
                      origin: "CFR",
                      category: "general-cfraddons",
                    },
                    type: "OPEN_PREFERENCES_PAGE",
                  },
                },
              ],
            },
            category: "cfrAddons",
            bucket_id: "CFR_M1",
            info_icon: {
              label: {
                string_id: "cfr-doorhanger-extension-sumo-link",
              },
              sumo_path: "extensionrecommendations",
            },
            heading_text: "Welcome to the experiment",
            notification_text: {
              string_id: "cfr-doorhanger-extension-notification2",
            },
          },
          trigger: {
            id: "openURL",
            params: [
              "www.facebook.com",
              "facebook.com",
              "www.instagram.com",
              "instagram.com",
              "www.whatsapp.com",
              "whatsapp.com",
              "web.whatsapp.com",
              "www.messenger.com",
              "messenger.com",
            ],
          },
          template: "cfr_doorhanger",
          frequency: {
            lifetime: 3,
          },
          targeting: "true",
        },
      },
    },
  ],
  userFacingName: "About:Welcome Pull Factor Reinforcement",
  isEnrollmentPaused: false,
  experimentDocumentUrl:
    "https://experimenter.services.mozilla.com/experiments/aboutwelcome-pull-factor-reinforcement/",
  userFacingDescription:
    "This study uses 4 different variants of about:welcome with a goal of testing new experiment framework and get insights on whether reinforcing pull-factors improves retention.",
});

add_task(async function test_loading_experimentsAPI() {
  // Force the WNPanel provider cache to 0 by modifying updateCycleInMs
  await SpecialPowers.pushPrefEnv({
    set: [
      [
        "browser.newtabpage.activity-stream.asrouter.providers.messaging-experiments",
        `{"id":"messaging-experiments","enabled":true,"type":"remote-experiments","messageGroups":["cfr","whats-new-panel","moments-page","snippets","cfr-fxa"],"updateCycleInMs":0}`,
      ],
    ],
  });
  const client = RemoteSettings("nimbus-desktop-experiments");
  await client.db.importChanges(
    {},
    42,
    [
      // Modify targeting to ensure the messages always show up
      { ...EXPERIMENT_PAYLOAD },
    ],
    { clear: true }
  );
  // Fetch the new recipe from RS
  await RemoteSettingsExperimentLoader.updateRecipes();
  await BrowserTestUtils.waitForCondition(
    () => ExperimentAPI.getExperiment({ featureId: "cfr" }),
    "ExperimentAPI should return an experiment"
  );

  const telemetryFeedInstance = new TelemetryFeed();
  Assert.ok(
    telemetryFeedInstance.isInCFRCohort,
    "Telemetry should return true"
  );

  // Reload the provider
  await ASRouter._updateMessageProviders();
  // Wait to load the messages from the messaging-experiments provider
  await ASRouter.loadMessagesFromAllProviders();

  Assert.ok(ASRouter.state.messages.find(m => m.id === "xman_test_message"));

  await client.db.clear();
  await SpecialPowers.popPrefEnv();
  // Reload the provider
  await ASRouter._updateMessageProviders();
});
