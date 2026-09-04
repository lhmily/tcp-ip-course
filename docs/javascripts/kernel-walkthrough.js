(() => {
  "use strict";

  const roots = document.querySelectorAll("[data-kernel-walkthrough]");
  if (!roots.length) return;

  const asString = (value, fallback = "") =>
    typeof value === "string" ? value : fallback;
  const titleCase = (value) =>
    asString(value, "unspecified")
      .replace(/[_-]+/g, " ")
      .replace(/\b\w/g, (letter) => letter.toUpperCase());

  roots.forEach((root) => {
    const dataElement = root.querySelector("[data-walkthrough-data]");
    if (!dataElement) return;

    let data;
    try {
      data = JSON.parse(dataElement.textContent);
    } catch (_error) {
      return;
    }

    const symbols = Array.isArray(data.symbols) ? data.symbols : [];
    const edges = Array.isArray(data.edges) ? data.edges : [];
    const symbolById = new Map(symbols.map((symbol) => [symbol.id, symbol]));
    const routes = {
      ingress: Array.isArray(data.routes?.ingress) ? data.routes.ingress.slice() : [],
      egress: Array.isArray(data.routes?.egress) ? data.routes.egress.slice() : [],
    };
    if (!routes.ingress.length || !routes.egress.length) return;

    const phaseLabels = new Map(
      (Array.isArray(data.phases) ? data.phases : []).map((item) => [item.key, item.label]),
    );
    const routeControls = Array.from(root.querySelectorAll("[data-route]"));
    const phaseControls = Array.from(root.querySelectorAll("[data-phase]"));
    const routeLists = Array.from(root.querySelectorAll("[data-route-list]"));
    const previous = root.querySelector('[data-step="previous"]');
    const next = root.querySelector('[data-step="next"]');
    const status = root.querySelector("[data-walkthrough-status]");
    const detail = root.querySelector("[data-walkthrough-detail]");
    if (!previous || !next || !status || !detail) return;

    const nodes = Array.from(root.querySelectorAll("[data-symbol-id]"));
    const nodeByRouteAndId = new Map(
      nodes.map((node) => [
        `${node.dataset.routeName}:${Number(node.dataset.symbolId)}`,
        node,
      ]),
    );

    let routeName = "ingress";
    let phase = "all";
    let selectedId = routes.ingress[0];

    const memberships = (symbol) =>
      Array.isArray(symbol?.memberships) ? symbol.memberships : [];
    const isInRoute = (symbol, name) =>
      routes[name].includes(symbol?.id) || memberships(symbol).includes(name);
    const visibleIds = () =>
      routes[routeName].filter((id) => {
        const symbol = symbolById.get(id);
        return symbol && (phase === "all" || symbol.phase === phase);
      });
    const selectedIndex = () => visibleIds().indexOf(selectedId);
    const edgeFor = (id) => {
      const route = routes[routeName];
      const index = route.indexOf(id);
      if (index < 0) return null;
      const previousId = index > 0 ? route[index - 1] : null;
      const nextId = index + 1 < route.length ? route[index + 1] : null;
      return (
        edges.find(
          (edge) =>
            edge.route === routeName && edge.from === previousId && edge.to === id,
        ) ||
        edges.find(
          (edge) => edge.route === routeName && edge.from === id && edge.to === nextId,
        ) ||
        null
      );
    };

    const setDetail = (symbol) => {
      const edge = edgeFor(symbol.id);
      const edgeType = titleCase(edge?.type || "route start");
      const edgeExplanation = asString(
        edge?.explanation,
        "This is the first indexed stop on the selected route.",
      );
      const role = asString(symbol.role, "Authored source waypoint");
      const ownership = asString(symbol.ownership, "Inspect the local source contract");
      const description = asString(symbol.description, "No description supplied.");
      const layer = asString(symbol.layer, "Unspecified layer");
      const phaseLabel = asString(phaseLabels.get(symbol.phase), titleCase(symbol.phase));
      const source = detail.querySelector("[data-detail-source]");

      detail.querySelector("[data-detail-name]").textContent = asString(
        symbol.name,
        symbol.key,
      );
      detail.querySelector("[data-detail-role]").textContent = role;
      detail.querySelector("[data-detail-description]").textContent = description;
      detail.querySelector("[data-detail-ownership]").textContent = ownership;
      detail.querySelector("[data-detail-edge-type]").textContent = edgeType;
      detail.querySelector("[data-detail-edge]").textContent = edgeExplanation;
      detail.querySelector("[data-detail-layer]").textContent = layer;
      detail.querySelector("[data-detail-phase]").textContent = phaseLabel;
      if (source) {
        source.href = symbol.source_url || "#";
        source.textContent = `${asString(symbol.path)}:${symbol.line}`;
      }
    };

    const ensureSelection = () => {
      const ids = visibleIds();
      if (!ids.length) return null;
      if (!ids.includes(selectedId)) selectedId = ids[0];
      return symbolById.get(selectedId) || null;
    };

    const render = ({ announce = false, focus = false } = {}) => {
      const symbol = ensureSelection();
      const ids = visibleIds();
      const index = selectedIndex();

      routeControls.forEach((control) => {
        control.setAttribute("aria-pressed", String(control.dataset.route === routeName));
      });
      phaseControls.forEach((control) => {
        control.setAttribute("aria-pressed", String(control.dataset.phase === phase));
      });
      routeLists.forEach((list) => {
        list.hidden = list.dataset.routeList !== routeName;
      });
      nodes.forEach((node) => {
        const nodeId = Number(node.dataset.symbolId);
        const nodeSymbol = symbolById.get(nodeId);
        const belongs = isInRoute(nodeSymbol, routeName);
        const matches = phase === "all" || nodeSymbol?.phase === phase;
        const active = belongs && nodeId === selectedId;
        node.closest("li").hidden = !(belongs && matches);
        node.setAttribute("aria-pressed", String(active));
        if (active) node.setAttribute("aria-current", "step");
        else node.removeAttribute("aria-current");
        node.tabIndex = active ? 0 : -1;
      });

      previous.disabled = index <= 0;
      next.disabled = index < 0 || index >= ids.length - 1;
      if (!symbol) {
        detail.hidden = true;
        status.textContent = `No ${routeName} symbols match this phase.`;
        previous.disabled = true;
        next.disabled = true;
        return;
      }

      detail.hidden = false;
      setDetail(symbol);
      status.textContent = announce
        ? `${asString(symbol.name, symbol.key)}, step ${index + 1} of ${ids.length} on the ${routeName} route.`
        : `${titleCase(routeName)} route, ${ids.length} visible symbols.`;
      if (focus) {
        nodeByRouteAndId
          .get(`${routeName}:${selectedId}`)
          ?.focus({ preventScroll: true });
      }
    };

    const selectAt = (index, focus = true) => {
      const ids = visibleIds();
      if (!ids.length) return;
      selectedId = ids[Math.max(0, Math.min(index, ids.length - 1))];
      render({ announce: true, focus });
    };

    routeControls.forEach((control) => {
      control.addEventListener("click", () => {
        routeName = control.dataset.route;
        selectedId = routes[routeName][0];
        render({ announce: true });
      });
    });
    phaseControls.forEach((control) => {
      control.addEventListener("click", () => {
        phase = control.dataset.phase;
        render({ announce: true });
      });
    });
    previous.addEventListener("click", () => selectAt(selectedIndex() - 1, false));
    next.addEventListener("click", () => selectAt(selectedIndex() + 1, false));

    nodes.forEach((node) => {
      node.addEventListener("click", () => {
        routeName = node.dataset.routeName;
        selectedId = Number(node.dataset.symbolId);
        render({ announce: true });
      });
      node.addEventListener("keydown", (event) => {
        const ids = visibleIds();
        let index = selectedIndex();
        if (event.key === "ArrowLeft") index -= 1;
        else if (event.key === "ArrowRight") index += 1;
        else if (event.key === "Home") index = 0;
        else if (event.key === "End") index = ids.length - 1;
        else if (event.key === "Enter" || event.key === " ") index = selectedIndex();
        else return;
        event.preventDefault();
        selectAt(index);
      });
    });

    root.classList.add("is-enhanced");
    render();
  });
})();
