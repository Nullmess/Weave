/** Browser-only preview bridge. Actual USB, device writes and system updates stay native. */
if (!window.weave) {
  const nativeOnly = async (): Promise<never> => {throw Error('This action requires Weave desktop (python3 fbt.py app).');};
  const detached = () => () => {};
  window.weave = {
    scanUsb: async () => [],
    connectUsb: nativeOnly, sendUsb: nativeOnly, disconnectUsb: async () => {},
    gateway: nativeOnly,
    onUsb: detached, onStatus: detached,
    saveFile: nativeOnly, openFile: nativeOnly,
    catalog: async (query: string, offset: number) => {
      const url = new URL('https://catalog.flipperzero.one/api/v0/application');
      url.searchParams.set('limit','100');url.searchParams.set('offset',String(offset));
      if (query) url.searchParams.set('search',query);
      const response = await fetch(url, {headers:{Accept:'application/json'}});
      if (!response.ok) throw Error(`Catalog HTTP ${response.status}`);
      return response.json();
    },
    catalogManifests: async () => {throw Error('Manifest discovery requires the desktop companion.');},
    catalogDetail: async () => {throw Error('Catalog details require the desktop companion.');},
    catalogVisual: async () => {throw Error('Catalog icons require the desktop companion.');},
    openUrl: async (url: string) => {
      const parsed = new URL(url);
      if (parsed.protocol !== 'https:' || !['github.com','catalog.flipperzero.one','lab.flipper.net','ko-fi.com'].includes(parsed.hostname))
        throw Error('Untrusted external URL');
      window.open(parsed.toString(),'_blank','noopener,noreferrer');
    },
    setStartOnBoot: async () => false,
    checkUpdate: nativeOnly, downloadUpdate: nativeOnly,
    cachedImage: async () => '', onUpdateProgress: detached, onFapProgress: detached,
    downloadFap: nativeOnly,
  } as typeof window.weave;
}
