import FullscreenCentered from "@/components/FullscreenCentered";
import Tab from "@/components/Tab";
import { LucasConnection, SERVICE_UUID } from "@/lib/LucasConnection";
import Heater from "@/pages/Heater";
import ConnectionContext from "@/stores/ConnectionContext";
import { BleClient } from "@capacitor-community/bluetooth-le";
import {
  IonApp,
  IonButton,
  IonLabel,
  IonSpinner,
  IonTab,
  IonTabBar,
  IonTabButton,
  IonTabs,
  isPlatform,
  setupIonicReact,
} from "@ionic/react";
import { useEffect, useState } from "react";

// Core CSS Imports for ionic
import "@ionic/react/css/core.css";
// System-controlled dark mode
import "@ionic/react/css/palettes/dark.system.css";

setupIonicReact();

export default function App() {
  const [bluetoothState, setBluetoothState] = useState<BluetoothState>({
    kind: State.Uninitialized,
  });

  useEffect(() => {
    switch (bluetoothState.kind) {
      case State.Uninitialized:
        BleClient.initialize()
          .then(() => {
            console.log("Bluetooth Initialized");
            setBluetoothState({
              kind: State.PrepareSearch,
            });
          })
          .catch(() => console.error("Failed to initialize bluetooth"));
        break;
      case State.PrepareSearch:
        if (!isPlatform("desktop")) {
          setTimeout(() => setBluetoothState({ kind: State.Searching }), 1000);
        }
        break;
      case State.Searching:
        BleClient.requestDevice({
          services: [SERVICE_UUID],
        })
          .then((device) =>
            BleClient.connect(device.deviceId, () => {
              console.warn("Disconnected from device, searching again");
              setBluetoothState({ kind: State.PrepareSearch });
            }).then(() => device),
          )
          .then((device) =>
            setBluetoothState({
              kind: State.Connected,
              lucasConnection: new LucasConnection(device),
            }),
          )
          .catch((error) => {
            console.error("Error connecting to device: ", error);
            setBluetoothState({ kind: State.PrepareSearch });
          });

        break;
      case State.Connected:
        break;
    }
  }, [bluetoothState]);

  switch (bluetoothState.kind) {
    case State.Uninitialized:
      return (
        <IonApp>
          <FullscreenCentered />
        </IonApp>
      );
    case State.PrepareSearch:
      if (isPlatform("desktop")) {
        return (
          <IonApp>
            <FullscreenCentered>
              <IonButton
                onClick={() =>
                  setBluetoothState({
                    kind: State.Searching,
                  })
                }
              >
                Conectar
              </IonButton>
            </FullscreenCentered>
          </IonApp>
        );
      }
    // fall through to reuse the same page on mobile
    case State.Searching:
      return (
        <IonApp>
          <FullscreenCentered>
            <IonSpinner />
          </FullscreenCentered>
        </IonApp>
      );
    case State.Connected:
      return (
        <IonApp>
          <ConnectionContext.Provider value={bluetoothState.lucasConnection}>
            <IonTabs>
              <Tab tab="heater">
                <Heater />
              </Tab>
              <Tab tab="empty"></Tab>

              <IonTabBar slot="bottom">
                <IonTabButton tab="heater">
                  <IonLabel>Aquecedor</IonLabel>
                </IonTabButton>
                <IonTabButton tab="empty">
                  <IonLabel>Vazio</IonLabel>
                </IonTabButton>
              </IonTabBar>

              <IonTab tab=""></IonTab>
            </IonTabs>
          </ConnectionContext.Provider>
        </IonApp>
      );
  }
}

enum State {
  Uninitialized,
  PrepareSearch,
  Searching,
  Connected,
}

interface Uninitialized {
  kind: State.Uninitialized;
}

interface WaitingForSearchTrigger {
  kind: State.PrepareSearch;
}

interface Searching {
  kind: State.Searching;
}

interface Connected {
  kind: State.Connected;
  lucasConnection: LucasConnection;
}

type BluetoothState =
  | Uninitialized
  | WaitingForSearchTrigger
  | Searching
  | Connected;
