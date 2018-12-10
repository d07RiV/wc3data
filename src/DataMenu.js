import React from 'react';
import { Route } from 'react-router-dom';
import { Nav, NavItem, NavDropdown, MenuItem } from 'react-bootstrap';
import { LinkContainer } from 'react-router-bootstrap';

import AppCache from 'data/cache';
import objectTypes from 'objects/types';

const ObjectMenuInner = ({match: {params: {build, type}}}) => (
  <NavDropdown eventKey="objects" title={objectTypes[type] || "Objects"} active={objectTypes[type] != null} id="objects-menu">
    {Object.keys(objectTypes).map(t => (
      <LinkContainer key={t} to={`/${build}/${t}`}>
        <MenuItem eventKey={`objects.${t}`}>
          <span className={`ObjectIcon ${t}`}/>{objectTypes[t]}
        </MenuItem>
      </LinkContainer>
    ))}
  </NavDropdown>
);

const ObjectMenu = () => <Route path="/:build/:type?" component={ObjectMenuInner}/>;

const DataMenu = () => (
  <AppCache.DataContext.Consumer>
    {data => (
      <Nav>
        <LinkContainer to={`/${data.id}`} exact>
          <NavItem eventKey="build">{data.name}</NavItem>
        </LinkContainer>
        {data.hasFile("listfile.txt") && (
          <LinkContainer to={`/${data.id}/files`}>
            <NavItem eventKey="files">Files</NavItem>
          </LinkContainer>
        )}
        <ObjectMenu/>
        {(data.hasFile("war3map.j") || data.hasFile("Scripts\\war3map.j")) && (
          <LinkContainer to={`/${data.id}/script`}>
            <NavItem eventKey="script">Script</NavItem>
          </LinkContainer>
        )}
      </Nav>
    )}
  </AppCache.DataContext.Consumer>
);

export default DataMenu;
