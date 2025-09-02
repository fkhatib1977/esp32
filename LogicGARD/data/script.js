let jsonData = {};
let fieldRefs = {};

document.addEventListener('DOMContentLoaded', () => {
  fetch('user.json')
    .then(res => res.json())
    .then(data => {
      jsonData = data;
      renderForm(data, document.getElementById('form-container'));
    });

  document.getElementById('save-btn').addEventListener('click', saveForm);
  document.getElementById('reset-btn').addEventListener('click', () => location.reload());
});

function renderForm(data, container, depth = 2, path = '') {
  for (const key in data) {
    const item = data[key];
    const fullPath = path ? `${path}_${key}` : key;

    if (item._hidden) continue;

    if (item._group) {
      const section = document.createElement('section');
      section.setAttribute('data-group', fullPath);

      const title = document.createElement(`h${depth}`);
      title.textContent = item._title || key;
      section.appendChild(title);

      if (item._collapsible) {
        title.style.cursor = 'pointer';
        section.classList.add('collapsible');
        if (item._collapsed) section.classList.add('collapsed');
        title.addEventListener('click', () => {
          section.classList.toggle('collapsed');
        });
      }

      renderForm(item, section, depth + 1, fullPath);
      container.appendChild(section);
    } else if (typeof item === 'object' && 'type' in item) {
      const field = createField(key, item, fullPath);
      container.appendChild(field);
    }
  }

  updateVisibility();
}

function createField(key, config, path) {
  if (!validateConfig(config)) return document.createTextNode(`Invalid config for ${key}`);

  switch (config.type) {
    case 'boolean':
      return createBooleanField(path, config);
    case 'select':
      return createSelectField(path, config);
    default:
      return createTextField(path, config);
  }
}

function createBooleanField(path, config) {
  const wrapper = document.createElement('div');
  wrapper.className = 'field boolean-field';

  const label = document.createElement('label');
  label.textContent = config.label || path;

  const toggle = document.createElement('div');
  toggle.className = 'toggle-switch';
  toggle.setAttribute('role', 'switch');
  toggle.setAttribute('tabindex', '0');
  toggle.setAttribute('aria-checked', config.value ? 'true' : 'false');
  toggle.setAttribute('data-checked', config.value ? 'true' : 'false');

  const error = document.createElement('div');
  error.className = 'error';

  let checked = !!config.value;

  toggle.addEventListener('click', () => {
    checked = !checked;
    toggle.setAttribute('aria-checked', checked ? 'true' : 'false');
    toggle.setAttribute('data-checked', checked ? 'true' : 'false');
    fieldRefs[path].input = checked;
    updateVisibility();
  });

  toggle.addEventListener('keydown', e => {
    if (e.key === ' ' || e.key === 'Enter') {
      e.preventDefault();
      toggle.click();
    }
  });

  wrapper.appendChild(label);
  wrapper.appendChild(toggle);
  wrapper.appendChild(error);

  fieldRefs[path] = { input: checked, config, error };
  return wrapper;
}

function createSelectField(path, config) {
  const wrapper = document.createElement('div');
  wrapper.className = 'field';

  const label = document.createElement('label');
  label.textContent = config.label || path;

  const select = document.createElement('select');

  let options = config.options;
  if (typeof options === 'string' && options.startsWith('$')) {
    options = resolveReference(jsonData, options);
  }

  if (!Array.isArray(options)) {
    const errorMsg = document.createElement('div');
    errorMsg.className = 'error';
    errorMsg.textContent = `Invalid options for ${path}`;
    wrapper.appendChild(label);
    wrapper.appendChild(errorMsg);
    return wrapper;
  }

  options.forEach(opt => {
    const option = document.createElement('option');
    option.value = opt;
    option.textContent = opt;
    select.appendChild(option);
  });

  select.value = config.value || '';
  select.dataset.path = path;

  const error = document.createElement('div');
  error.className = 'error';

  select.addEventListener('change', () => {
    fieldRefs[path].input = select.value; // ✅ Sync value
    updateVisibility();
  });

  wrapper.appendChild(label);
  wrapper.appendChild(select);
  wrapper.appendChild(error);

  fieldRefs[path] = { input: select.value, config, error }; // ✅ Initial value
  return wrapper;
}

function createTextField(path, config) {
  const wrapper = document.createElement('div');
  wrapper.className = 'field';

  const label = document.createElement('label');
  label.textContent = config.label || path;

  const inputWrapper = document.createElement('div');
  inputWrapper.className = 'input-wrapper';

  const input = document.createElement('input');
  input.type = config.type || 'text';
  input.value = config.value || '';
  input.dataset.path = path;

  input.addEventListener('change', () => {
    fieldRefs[path].input = input.value; // ✅ Sync value
    updateVisibility();
  });

  inputWrapper.appendChild(input);

  if (config.type === 'password') {
    const eye = document.createElement('span');
    eye.className = 'eye-icon';
    eye.innerHTML = '👁️';
    eye.title = 'Show/Hide Password';

    eye.addEventListener('click', () => {
      const isMasked = input.type === 'password';
      input.type = isMasked ? 'text' : 'password';
      eye.innerHTML = isMasked ? '🙈' : '👁️';
    });

    inputWrapper.appendChild(eye);
  }

  const error = document.createElement('div');
  error.className = 'error';

  wrapper.appendChild(label);
  wrapper.appendChild(inputWrapper);
  wrapper.appendChild(error);

  fieldRefs[path] = { input: input.value, config, error }; // ✅ Initial value
  return wrapper;
}

function validateConfig(config) {
  if (typeof config !== 'object') return false;
  if (!('type' in config)) return false;
  if (config.type === 'select' && !Array.isArray(config.options) && typeof config.options !== 'string') return false;
  return true;
}

function evaluateVisibleIf(expression) {
  const match = expression.match(/^(.+?)\s*==\s*['"]?(true|false|[^'"]+)['"]?$/);
  if (!match) return true;

  const [_, depPath, expectedRaw] = match;
  const expected = expectedRaw === 'true' ? true :
                   expectedRaw === 'false' ? false :
                   expectedRaw;

  const ref = fieldRefs[depPath.trim()];
  if (!ref) return false;

  const actual = ref.input;
  return actual == expected;
}

function updateVisibility() {
  document.querySelectorAll('[data-group]').forEach(group => {
    const path = group.getAttribute('data-group');
    const config = getConfigByPath(jsonData, path);
    if (config?.visibleIf) {
      const shouldShow = evaluateVisibleIf(config.visibleIf);
      group.style.display = shouldShow ? 'block' : 'none';
    }
  });

  for (const path in fieldRefs) {
    const { config } = fieldRefs[path];
    if (config.visibleIf) {
      const shouldShow = evaluateVisibleIf(config.visibleIf);
      const fieldEl = document.querySelector(`[data-path="${path}"]`)?.closest('.field');
      if (fieldEl) fieldEl.style.display = shouldShow ? '' : 'none';
    }
  }
}

function getConfigByPath(obj, path) {
  const parts = path.split('_');
  let current = obj;
  for (const part of parts) {
    if (current && typeof current === 'object') {
      current = current[part];
    } else {
      return null;
    }
  }
  return current;
}

function resolveReference(schema, ref) {
  if (typeof ref !== 'string' || !ref.startsWith('$')) return ref;

  const path = ref.slice(1).split('.');
  let current = schema;

  for (const key of path) {
    if (current && typeof current === 'object' && key in current) {
      current = current[key];
    } else {
      console.warn(`Reference "${ref}" could not be resolved.`);
      return null;
    }
  }

  return current;
}

function saveForm() {
  let valid = true;
  const updated = {};

  updated["isConfigured"] = true;

  for (const path in fieldRefs) {
    const { input, config, error } = fieldRefs[path];
    let value = input;

    // Normalize value based on input type
    if (input instanceof HTMLInputElement || input instanceof HTMLSelectElement) {
      value = input.value?.trim?.() ?? '';
    }

    error.textContent = '';

    const isEmpty = val => val === '' || val === null || val === undefined;
    if (config.required && isEmpty(value)) {
      error.textContent = 'This field is required.';
      valid = false;
    }

    updated[path] = value;
  }

  // 🚀 Submit if valid
  if (valid) {
    fetch('/save-user', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(updated)
    }).then(() => {
      alert('Configuration saved.');
    }).catch(err => {
      console.error('Save failed:', err);
      alert('Failed to save configuration.');
    });
  }
}